//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"

#include <algorithm>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>

#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"
#include "arm_kinematics/joint_map/materialize_joint_map.hpp"
#include "arm_kinematics/joint_map/missing_input_resolution.hpp"
#include "arm_kinematics/joint_map/transmission_reachability.hpp"

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

  // Step 1: Reachability analysis — pass defs directly, no SID pre-registration needed.
  const auto reach = TransmissionReachability::analyze(transmission_analysis_, inputs);

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

  // Step 4: Plan and materialize.
  const auto blueprint = plan_joint_map(reach, outputs);
  return materialize_joint_map(blueprint, transmission_analysis_);
}

}  // namespace arm_kinematics
