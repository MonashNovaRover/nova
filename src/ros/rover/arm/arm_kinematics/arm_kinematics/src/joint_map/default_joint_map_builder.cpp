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
    return tl::unexpected(JointMapBuildError::UnknownJoint{std::move(unique)});
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
    return tl::unexpected(JointMapBuildError::MissingInputs{
      diag.unproducible,
      std::move(diag.resolutions),
    });
  }

  // Step 4: Plan and materialize.
  const auto blueprint = plan_joint_map(reach, outputs);
  return materialize_joint_map(blueprint, transmission_analysis_);
}

}  // namespace arm_kinematics
