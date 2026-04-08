//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP
#define ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP

#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Default builder backed by a `TransmissionAnalysis`.
 *
 * \note This class is a stub during step 2 of the state-interface refactor. The legacy
 * `build_expected(input_names, output_names, JointQuantity)` API and the `with_urdf` / `with_transmissions` /
 * `with_transmission_model` setup methods have been removed; they all delegated to deleted plan-based machinery.
 *
 * Step 3 of the refactor will finalize the new `JointMapBuilder` virtual signature
 * (taking `span<const StateInterfaceId>` and returning `tl::expected<JointMap, JointMapBuildError>`).
 * Step 6 will give this class its real implementation, which constructs a `TransmissionReachability`
 * from the request, runs `diagnose_missing_outputs` against it, and (on success) walks a
 * `JointMapBlueprint` to emit a runtime `JointMap`.
 */
class ARM_KINEMATICS_PUBLIC DefaultJointMapBuilder {
public:
  DefaultJointMapBuilder() = default;
  ~DefaultJointMapBuilder() = default;

  [[nodiscard]] const TransmissionAnalysis & get_transmission_analysis() const noexcept
  {
    return transmission_analysis_;
  }

  [[nodiscard]] TransmissionAnalysis & get_mutable_transmission_analysis() noexcept
  {
    return transmission_analysis_;
  }

private:
  TransmissionAnalysis transmission_analysis_{};
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP
