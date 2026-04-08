//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP
#define ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP

#include "arm_kinematics/joint_map/joint_map_builder.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Default builder backed by a `TransmissionAnalysis`.
 *
 * `build_expected` runs the canonical analysis-time pipeline:
 *   1. `TransmissionReachability::analyze(analysis_, inputs)` — eager forward fixed point.
 *   2. `diagnose_missing_outputs(reach, outputs)` — classify each requested output as
 *      satisfied / unreachable / ambiguous (direct or transitive).
 *   3. If the diagnosis is non-empty: return a `JointMapBuildError` with the relevant slice.
 *      Ambiguity wins over unreachability when both apply (the user fixes the ambiguity
 *      first, retries, then sees any remaining unreachables).
 *   4. Otherwise: `plan_joint_map(reach, outputs)` to produce a blueprint, then
 *      `materialize_joint_map(blueprint, analysis_)` to obtain the runtime `JointMap`.
 *
 * The builder owns its `TransmissionAnalysis`. Populate it via `get_mutable_transmission_analysis()`
 * before calling `build_expected`.
 */
class ARM_KINEMATICS_PUBLIC DefaultJointMapBuilder : public JointMapBuilder {
public:
  DefaultJointMapBuilder() = default;
  ~DefaultJointMapBuilder() override = default;

  [[nodiscard]] const TransmissionAnalysis & get_transmission_analysis() const noexcept
  {
    return transmission_analysis_;
  }

  [[nodiscard]] TransmissionAnalysis & get_mutable_transmission_analysis() noexcept
  {
    return transmission_analysis_;
  }

  [[nodiscard]] tl::expected<JointMap, JointMapBuildError> build_expected(
    span<const StateInterfaceId> inputs,
    span<const StateInterfaceId> outputs) const override;

private:
  TransmissionAnalysis transmission_analysis_{};
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP
