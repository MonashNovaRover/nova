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
 * Default builder backed by a `const TransmissionAnalysis &` (non-owning).
 *
 * `build_expected` runs the canonical analysis-time pipeline:
 *   1. `TransmissionReachability::analyze(analysis_, inputs)` — eager forward fixed point.
 *   2. `diagnose_missing_outputs(reach, outputs)` — classify each requested output as
 *      satisfied / unproducible / ambiguous (directly or transitively blocked).
 *   3. If the diagnosis is non-empty: return a `JointMapBuildError` with the relevant slice.
 *      Ambiguity wins over unproducibility when both apply (the user fixes the ambiguity
 *      first, retries, then sees any remaining unproducible outputs).
 *   4. Otherwise: `plan_joint_map(reach, outputs)` to produce a blueprint, then
 *      `materialize_joint_map(blueprint, analysis_)` to obtain the runtime `JointMap`.
 *
 * **Ownership.** The builder does NOT own the `TransmissionAnalysis` — it stores a const
 * reference passed at construction. The caller (typically `RobotModel` or a test fixture) is
 * responsible for populating the analysis BEFORE constructing the builder, and keeping it
 * alive for the builder's lifetime. `build_expected` only reads the analysis; it never
 * mutates it.
 */
class ARM_KINEMATICS_PUBLIC DefaultJointMapBuilder : public JointMapBuilder {
public:
  /// Construct a builder around a caller-owned, fully-populated `TransmissionAnalysis`.
  explicit DefaultJointMapBuilder(const TransmissionAnalysis & analysis) noexcept
  : transmission_analysis_(analysis)
  {
  }

  ~DefaultJointMapBuilder() override = default;

  [[nodiscard]] const TransmissionAnalysis & get_transmission_analysis() const noexcept
  {
    return transmission_analysis_;
  }

  [[nodiscard]] tl::expected<JointMap, JointMapBuildError> build_expected(
    span<const StateInterfaceId> inputs,
    span<const StateInterfaceId> outputs) const override;

private:
  const TransmissionAnalysis & transmission_analysis_;
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP
