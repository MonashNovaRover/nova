//
// Created by Bailey Chessum on 21/03/2026.
//

#ifndef ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP
#define ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP

#include <memory>
#include <string>
#include <vector>

#include <rclcpp/logger.hpp>
#include <urdf/model.h>

#include "arm_kinematics/joint_map/joint_map_builder.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_analysis_import.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/joint_map/transmission_plan.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Default shared builder backed by the robot model's URDF-derived metadata.
 *
 * Current behavior:
 *   - gathers mimic-joint affine transmissions from the URDF
 *   - parses ros2_control transmission definitions
 *   - caches a TransmissionAnalysis built from normalized transmission models
 *   - delegates affine and grouped runtime construction to TransmissionAnalysisJointMapBuilder
 *
 * This type is the default shared implementation, not the only possible implementation. FK plugins may return a
 * different JointMapBuilder when they need backend-specific mapping behavior.
 */
class ARM_KINEMATICS_PUBLIC DefaultJointMapBuilder final : public JointMapBuilder {
public:
  DefaultJointMapBuilder() = default;

  /**
   * Constructs a JointMap that maps input_names to output_names for the requested quantity.
   *
   * This builder remains an orchestration layer only. It converts boundary names to canonical JointIds and then
   * delegates to the affine or grouped planning/compilation path as appropriate.
   */
  [[nodiscard]] tl::expected<JointMap, std::string> build_expected(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names,
    JointQuantity quantity) const override;

  /**
   * Uses the given URDF model to add mimic-joint affine transmissions to the cached TransmissionAnalysis.
   *
   * \param urdf_model The urdf::Model to get mimic joints from.
   */
  DefaultJointMapBuilder & with_urdf(const urdf::Model & urdf_model);

  /**
   * Manually parses the given URDF string to get transmission definitions to include in the builder.
   *
   * \note This overload gracefully fails with a log message rather than throwing an exception. No transmissions are
   *       added in the case of an exception occurring.
   *
   * \param urdf_string The URDF as a string, containing a ros2_control definition.
   * \param logger the logger to log any caught exceptions to.
   */
  DefaultJointMapBuilder & with_transmissions(const std::string & urdf_string, rclcpp::Logger logger);

  /**
   * Manually parses the given URDF string to get transmission definitions to include in the builder.
   *
   * \param urdf_string The URDF as a string, containing a ros2_control definition.
   * \throws std::runtime_error for invalid URDFs.
   */
  DefaultJointMapBuilder & with_transmissions_dangerous(const std::string & urdf_string);

  /**
   * Adds a normalized transmission model to the cached transmission analysis.
   */
  DefaultJointMapBuilder & with_transmission_model(std::unique_ptr<TransmissionModel> model);

  [[nodiscard]] const TransmissionAnalysis & get_transmission_analysis() const noexcept;

private:
  /// Parsed transmission metadata from ros2_control XML for future use and diagnostics.
  std::vector<hardware_interface::TransmissionInfo> transmissions_{};
  /// Cached structural transmission analysis derived from registered transmission models.
  TransmissionAnalysis transmission_analysis_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP
