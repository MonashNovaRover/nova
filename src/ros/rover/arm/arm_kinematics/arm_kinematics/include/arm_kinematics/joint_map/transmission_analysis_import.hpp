//
// Created by Bailey Chessum on 25/03/2026.
//

#ifndef ARM_KINEMATICS_TRANSMISSION_ANALYSIS_IMPORT_HPP
#define ARM_KINEMATICS_TRANSMISSION_ANALYSIS_IMPORT_HPP

#include <string>
#include <vector>

#include <hardware_interface/component_parser.hpp>
#include <rclcpp/logger.hpp>
#include <urdf/model.h>

#include "arm_kinematics/joint_map/ros2_control_transmission_plugin_loader.hpp"
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/visibility_control.h"

namespace arm_kinematics {

/**
 * Parses ros2_control transmission metadata from a URDF string and adds the resulting transmission topology to the
 * given TransmissionAnalysis.
 *
 * \param transmission_analysis The analysis object to modify.
 * \param urdf_string The URDF as a string, containing a ros2_control definition.
 * \returns The parsed ros2_control transmission metadata.
 * \throws std::runtime_error for invalid URDFs.
 */
[[nodiscard]] ARM_KINEMATICS_PUBLIC std::vector<hardware_interface::TransmissionInfo>
add_ros2_control_transmissions_to_analysis_dangerous(
  TransmissionAnalysis & transmission_analysis,
  const std::string & urdf_string,
  std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader =
    std::make_shared<Ros2ControlTransmissionPluginLoader>());

/**
 * Parses ros2_control transmission metadata from a URDF string and adds the resulting transmission topology to the
 * given TransmissionAnalysis.
 *
 * \note This overload gracefully fails with a log message rather than throwing an exception. No transmissions are
 *       added in the case of an exception occurring.
 *
 * \param transmission_analysis The analysis object to modify.
 * \param urdf_string The URDF as a string, containing a ros2_control definition.
 * \param logger The logger to log any caught exceptions to.
 * \returns The parsed ros2_control transmission metadata. Returns an empty vector on failure.
 */
[[nodiscard]] ARM_KINEMATICS_PUBLIC std::vector<hardware_interface::TransmissionInfo>
add_ros2_control_transmissions_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const std::string & urdf_string,
  std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader,
  rclcpp::Logger logger);

[[nodiscard]] ARM_KINEMATICS_PUBLIC std::vector<hardware_interface::TransmissionInfo>
add_ros2_control_transmissions_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const std::string & urdf_string,
  rclcpp::Logger logger);

/**
 * Adds normalized affine transmission relationships for all mimic joints in the given URDF model.
 *
 * Mimic chains are reduced during import so each stored affine transmission maps from the original source joint to the
 * final mimic joint directly.
 *
 * \param transmission_analysis The analysis object to modify.
 * \param urdf_model The URDF model to import mimic relationships from.
 * \throws std::runtime_error for invalid mimic chains or cycles.
 */
ARM_KINEMATICS_PUBLIC void add_mimic_transmissions_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const urdf::Model & urdf_model);

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TRANSMISSION_ANALYSIS_IMPORT_HPP
