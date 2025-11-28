//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_KINEMATICS_PARAMS_HPP
#define ARM_KINEMATICS_KINEMATICS_PARAMS_HPP

#include <string>
#include <arm_kinematics/visibility_control.h>
#include <rclcpp/node_interfaces/node_parameters_interface.hpp>
#include <urdf/model.h>

namespace arm_kinematics {

/**
 * Parameters used to configure kinematics plugins
 */
struct ARM_KINEMATICS_PUBLIC KinematicsParams {
  using NodeParametersInterface = rclcpp::node_interfaces::NodeParametersInterface;
  using SharedPtr = std::shared_ptr<KinematicsParams>;

  /// The name of the link in the URDF to treat as the 'origin' for kinematics
  std::string base_link_name;

  /// The name of the link to use by default as the target in FK and IK calculations. Usually the end effector.
  std::string ee_link_name;

  /// URDF describing the robot
  const std::string & robot_description;

  /// Lazily evaluated urdf model from parsing robot_description
  [[nodiscard]] const urdf::Model & get_urdf_model() const;

  /**
   * Constructor
   * @param node The parameter interface to get params from
   * @param robot_description The URDF string used in kinematics
   */
  explicit KinematicsParams(
    const NodeParametersInterface::SharedPtr & node,
    const std::string & robot_description);

private:
  /// Lazily evaluated URDF model retrieved from get_urdf_model()
  /// I added this here so multiple plugins could share the same URDF, and we wouldn't need to parse it more than once
  mutable std::unique_ptr<urdf::Model> urdf_model_;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_KINEMATICS_PARAMS_HPP
