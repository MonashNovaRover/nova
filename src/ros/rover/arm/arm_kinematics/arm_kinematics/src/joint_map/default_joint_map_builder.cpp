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

constexpr std::size_t kMaxFormattedInterfaces = 5;

std::string format_state_interface(
  const TransmissionAnalysis & analysis,
  const StateInterfaceDefinition & def)
{
  const auto & joint_names = analysis.joint_order().inverse;
  std::string joint_name;
  if (def.joint_id < joint_names.size()) {
    joint_name = joint_names[def.joint_id];
  } else {
    joint_name = "<joint_id=" + std::to_string(def.joint_id) + ">";
  }
  return joint_name + "/" + def.interface_id.name;
}

std::string format_interface_list(
  const TransmissionAnalysis & analysis,
  const std::vector<StateInterfaceDefinition> & defs)
{
  std::ostringstream oss;
  oss << "[";
  const std::size_t shown = std::min(defs.size(), kMaxFormattedInterfaces);
  for (std::size_t i = 0; i < shown; ++i) {
    if (i > 0) oss << ", ";
    oss << format_state_interface(analysis, defs[i]);
  }
  if (defs.size() > shown) {
    oss << ", ...and " << (defs.size() - shown) << " more";
  }
  oss << "]";
  return oss.str();
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
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::UnknownJoint;
    err.unknown_joints = std::move(unique);

    std::ostringstream oss;
    oss << "DefaultJointMapBuilder: " << err.unknown_joints.size()
        << " JointId(s) in the request are not registered in the analysis: [";
    const std::size_t shown = std::min(err.unknown_joints.size(), kMaxFormattedInterfaces);
    for (std::size_t i = 0; i < shown; ++i) {
      if (i > 0) oss << ", ";
      oss << "joint_id=" << err.unknown_joints[i];
    }
    if (err.unknown_joints.size() > shown) {
      oss << ", ...and " << (err.unknown_joints.size() - shown) << " more";
    }
    oss << "]";
    err.message = oss.str();
    return tl::unexpected(std::move(err));
  }

  // Step 1: Reachability analysis — pass defs directly, no SID pre-registration needed.
  const auto reach = TransmissionReachability::analyze(transmission_analysis_, inputs);

  // Step 2: Diagnose against the requested outputs.
  const auto diag = diagnose_missing_outputs(reach, outputs);

  // Step 3: Surface errors. Ambiguity wins over MissingInputs.
  const bool has_ambiguity = !diag.directly_ambiguous_outputs.empty() ||
                             !diag.transitively_blocked_outputs.empty();
  if (has_ambiguity) {
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::Ambiguous;
    err.ambiguous_interfaces = std::move(diag.relevant_blocking_ambiguities);

    std::vector<StateInterfaceDefinition> affected;
    affected.reserve(
      diag.directly_ambiguous_outputs.size() + diag.transitively_blocked_outputs.size());
    affected.insert(
      affected.end(),
      diag.directly_ambiguous_outputs.begin(),
      diag.directly_ambiguous_outputs.end());
    affected.insert(
      affected.end(),
      diag.transitively_blocked_outputs.begin(),
      diag.transitively_blocked_outputs.end());

    std::ostringstream oss;
    oss << "DefaultJointMapBuilder: " << affected.size()
        << " requested output(s) cannot be produced due to ambiguity";
    if (!diag.directly_ambiguous_outputs.empty() && !diag.transitively_blocked_outputs.empty()) {
      oss << " (" << diag.directly_ambiguous_outputs.size() << " directly ambiguous, "
          << diag.transitively_blocked_outputs.size()
          << " transitively blocked by upstream ambiguity)";
    } else if (!diag.transitively_blocked_outputs.empty()) {
      oss << " (transitively blocked by upstream ambiguity)";
    }
    oss << ". Affected outputs: " << format_interface_list(transmission_analysis_, affected);
    if (!err.ambiguous_interfaces.empty()) {
      std::vector<StateInterfaceDefinition> ambiguous_defs;
      ambiguous_defs.reserve(err.ambiguous_interfaces.size());
      for (const auto & a : err.ambiguous_interfaces) {
        ambiguous_defs.push_back(a.interface);
      }
      oss << ". Upstream ambiguous interfaces: "
          << format_interface_list(transmission_analysis_, ambiguous_defs);
    }
    err.message = oss.str();
    return tl::unexpected(std::move(err));
  }
  if (!diag.unproducible.empty()) {
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::MissingInputs;
    err.unproducible_outputs = diag.unproducible;
    err.resolutions = std::move(diag.resolutions);
    err.message = "DefaultJointMapBuilder: " + std::to_string(err.unproducible_outputs.size()) +
                  " requested output(s) are not derivable from the supplied inputs: " +
                  format_interface_list(transmission_analysis_, err.unproducible_outputs);
    return tl::unexpected(std::move(err));
  }

  // Step 4: Plan and materialize.
  const auto blueprint = plan_joint_map(reach, outputs);
  return materialize_joint_map(blueprint, transmission_analysis_);
}

}  // namespace arm_kinematics
