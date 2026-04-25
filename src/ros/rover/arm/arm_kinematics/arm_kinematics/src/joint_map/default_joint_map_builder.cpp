//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "arm_kinematics/joint_map/constant_supplemented_joint_map.hpp"
#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"
#include "arm_kinematics/joint_map/materialize_joint_map.hpp"
#include "arm_kinematics/joint_map/missing_input_resolution.hpp"
#include "arm_kinematics/joint_map/transmission_reachability.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"

namespace arm_kinematics {

namespace {

NamedStateInterfaceDefinition to_named(
  const TransmissionAnalysis & analysis,
  const StateInterfaceDefinition & def)
{
  return {analysis.joint_order().inverse[def.joint_id], def.interface_id};
}

NamedMissingInputResolution to_named(
  const TransmissionAnalysis & analysis,
  const MissingInputResolution & res)
{
  std::vector<std::vector<NamedStateInterfaceDefinition>> named_alternatives;
  named_alternatives.reserve(res.transmission_alternatives.size());
  for (const auto & alt : res.transmission_alternatives) {
    auto & named_alt = named_alternatives.emplace_back();
    named_alt.reserve(alt.size());
    for (const auto & def : alt) {
      named_alt.push_back(to_named(analysis, def));
    }
  }
  return {to_named(analysis, res.missing), std::move(named_alternatives), res.affine_root};
}

}  // namespace

tl::expected<JointMap, JointMapBuildError> DefaultJointMapBuilder::build_expected(
  const span<const StateInterfaceDefinition> inputs,
  const span<const StateInterfaceDefinition> outputs) const
{
  const std::size_t joint_count = transmission_analysis_.joint_order().inverse.size();

  // Step 0: Validate that every JointId in the request is registered in the analysis.
  std::vector<JointId> unknown_joints;
  for (const auto & def : inputs) {
    if (def.joint_id >= joint_count) {
      unknown_joints.push_back(def.joint_id);
    }
  }
  for (const auto & def : outputs) {
    if (def.joint_id >= joint_count) {
      unknown_joints.push_back(def.joint_id);
    }
  }
  if (!unknown_joints.empty()) {
    std::vector<JointId> unique;
    {
      std::unordered_set<JointId> seen;
      seen.reserve(unknown_joints.size());
      unique.reserve(unknown_joints.size());
      for (const auto jid : unknown_joints) {
        if (seen.insert(jid).second) {
          unique.push_back(jid);
        }
      }
    }
    std::vector<std::string> named;
    named.reserve(unique.size());
    for (const auto jid : unique) {
      if (jid < joint_count) {
        named.push_back(transmission_analysis_.joint_order().inverse[jid]);
      } else {
        named.push_back("<unknown id=" + std::to_string(jid) + ">");
      }
    }
    return tl::unexpected(JointMapBuildError::UnknownJoint{std::move(named)});
  }

  // Pre-step (A): Synthesize explicitly defaulted state interfaces as synthetic constant inputs.
  //
  // Only the interfaces listed in default_state_interface_values_ are synthesized.
  // No implicit expansion to other interfaces occurs.
  std::vector<StateInterfaceDefinition> synthetic_inputs;
  std::vector<double> constant_values;
  if (!default_state_interface_values_.empty()) {
    std::unordered_set<StateInterfaceDefinition> available_inputs(inputs.begin(), inputs.end());
    const auto has_real_input_for_affine_group =
      [&](const StateInterfaceDefinition & def) -> bool {
        if (transmission_analysis_.affine_projection_rule(def.interface_id) == nullptr) {
          return false;
        }

        const JointId root = transmission_analysis_.affine_root_of(def.joint_id);
        for (const auto & input : inputs) {
          if (input.interface_id != def.interface_id || input.joint_id >= joint_count) {
            continue;
          }
          if (transmission_analysis_.affine_root_of(input.joint_id) == root) {
            return true;
          }
        }
        return false;
      };

    const auto add_synthetic_input =
      [&](StateInterfaceDefinition def, const double value) {
        if (def.joint_id >= joint_count) {
          return;
        }
        if (available_inputs.count(def) || has_real_input_for_affine_group(def)) {
          return;
        }
        available_inputs.insert(def);
        synthetic_inputs.push_back(std::move(def));
        constant_values.push_back(value);
      };

    for (const auto & [def, value] : default_state_interface_values_) {
      add_synthetic_input(def, value);
    }
  }

  // (B) Build augmented inputs = real inputs ++ synthetic inputs.
  std::vector<StateInterfaceDefinition> augmented_storage;
  span<const StateInterfaceDefinition> effective_inputs = inputs;
  if (!synthetic_inputs.empty()) {
    augmented_storage.reserve(inputs.size() + synthetic_inputs.size());
    augmented_storage.insert(augmented_storage.end(), inputs.begin(), inputs.end());
    augmented_storage.insert(
      augmented_storage.end(), synthetic_inputs.begin(), synthetic_inputs.end());
    effective_inputs = {augmented_storage.data(), augmented_storage.size()};
  }

  // Step 1: Reachability analysis against effective inputs (augmented when defaults present).
  const auto reach = TransmissionReachability::analyze(transmission_analysis_, effective_inputs);

  // Step 2: Diagnose against the requested outputs.
  const auto diag = diagnose_missing_outputs(reach, outputs);

  // Step 3: Surface errors. Ambiguity wins over MissingInputs.
  const bool has_ambiguity = !diag.directly_ambiguous_outputs.empty() ||
                             !diag.transitively_blocked_outputs.empty();
  if (has_ambiguity) {
    return tl::unexpected(JointMapBuildError::Ambiguous{
      std::move(diag.relevant_blocking_ambiguities),
    });
  }
  if (!diag.unproducible.empty()) {
    std::vector<NamedStateInterfaceDefinition> named_unproducible;
    named_unproducible.reserve(diag.unproducible.size());
    for (const auto & def : diag.unproducible) {
      named_unproducible.push_back(to_named(transmission_analysis_, def));
    }
    std::vector<NamedMissingInputResolution> named_resolutions;
    named_resolutions.reserve(diag.resolutions.size());
    for (const auto & res : diag.resolutions) {
      named_resolutions.push_back(to_named(transmission_analysis_, res));
    }
    return tl::unexpected(JointMapBuildError::MissingInputs{
      std::move(named_unproducible),
      std::move(named_resolutions),
    });
  }

  // Step 4: Plan and materialize against effective inputs.
  const auto blueprint = plan_joint_map(reach, outputs);
  auto result = materialize_joint_map(blueprint, transmission_analysis_);

  // Post-step (G): wrap in ConstantSupplementedJointMap if synthetic inputs were injected.
  if (!synthetic_inputs.empty()) {
    return JointMap(ConstantSupplementedJointMap(
      std::move(result), std::move(constant_values), inputs.size()));
  }
  return result;
}

}  // namespace arm_kinematics
