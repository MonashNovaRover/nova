//
// Created by Bailey Chessum on 22/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_ANALYSIS_JOINT_MAP_BUILDER_HPP
#define ARM_KINEMATICS_TRANSMISSION_ANALYSIS_JOINT_MAP_BUILDER_HPP

#include "arm_kinematics/joint_map/joint_map_builder.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"

namespace arm_kinematics {

/**
 * JointMapBuilder implementation that consumes an existing TransmissionAnalysis.
 *
 * This is the reusable consumer for the shared affine and grouped planning/compilation helpers. It does not import
 * URDF data or own transmission metadata itself.
 */
class ARM_KINEMATICS_PUBLIC TransmissionAnalysisJointMapBuilder final : public JointMapBuilder {
public:
  explicit TransmissionAnalysisJointMapBuilder(const TransmissionAnalysis & transmission_analysis)
    : transmission_analysis_(transmission_analysis)
  {
  }

  /**
   * Constructs a JointMap from the cached TransmissionAnalysis.
   */
  [[nodiscard]] tl::expected<JointMap, std::string> build_expected(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names,
    JointQuantity quantity) const override;

  [[nodiscard]] const TransmissionAnalysis & get_transmission_analysis() const noexcept
  {
    return transmission_analysis_;
  }

private:
  const TransmissionAnalysis & transmission_analysis_;
};

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_ANALYSIS_JOINT_MAP_BUILDER_HPP
