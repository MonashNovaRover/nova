//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"

#include <utility>

#include "arm_kinematics/joint_map/joint_map_blueprint.hpp"
#include "arm_kinematics/joint_map/materialize_joint_map.hpp"
#include "arm_kinematics/joint_map/missing_input_resolution.hpp"
#include "arm_kinematics/joint_map/transmission_reachability.hpp"

namespace arm_kinematics {

tl::expected<JointMap, JointMapBuildError> DefaultJointMapBuilder::build_expected(
  const span<const StateInterfaceId> inputs,
  const span<const StateInterfaceId> outputs) const
{
  // Step 1: Reachability analysis. Output-independent — purely a function of the analysis +
  // inputs.
  auto reach = TransmissionReachability::analyze(transmission_analysis_, inputs);

  // Step 2: Diagnose against the requested outputs. This catches:
  //   - directly unreachable outputs (no producer)
  //   - directly ambiguous outputs (≥2 candidate producers)
  //   - transitively poisoned outputs (producer chain touches an ambiguous interface)
  auto diag = diagnose_missing_outputs(
    reach, span<const StateInterfaceId>(outputs.data_, outputs.size_));

  // Step 3: If anything is broken, surface it. Ambiguity wins over unreachability when both
  // apply.
  if (!diag.ambiguous_outputs.empty()) {
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::Ambiguous;
    err.message =
      "DefaultJointMapBuilder: requested outputs depend on ambiguous interfaces";
    err.ambiguous_interfaces = std::move(diag.relevant_ambiguities);
    return tl::unexpected(std::move(err));
  }
  if (!diag.unreachable.empty()) {
    JointMapBuildError err{};
    err.kind = JointMapBuildError::Kind::MissingInputs;
    err.message =
      "DefaultJointMapBuilder: one or more requested outputs are not derivable from the inputs";
    err.unreachable_outputs = std::move(diag.unreachable);
    err.resolutions = std::move(diag.resolutions);
    return tl::unexpected(std::move(err));
  }

  // Step 4: Plan and materialize. Both are total functions given the diagnosis cleared.
  const auto blueprint = plan_joint_map(
    reach, span<const StateInterfaceId>(outputs.data_, outputs.size_));
  return materialize_joint_map(blueprint, transmission_analysis_);
}

}  // namespace arm_kinematics
