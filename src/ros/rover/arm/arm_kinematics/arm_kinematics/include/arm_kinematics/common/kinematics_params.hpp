//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_KINEMATICS_PARAMS_HPP
#define ARM_KINEMATICS_KINEMATICS_PARAMS_HPP

#include <string>
#include <rclcpp/node_interfaces/node_interfaces.hpp>
#include <rclcpp/node_interfaces/node_parameters_interface.hpp>
#include <urdf/model.h>

#include "arm_kinematics/visibility_control.h"
#include "robot_model.hpp"

namespace arm_kinematics {

/**
 * Parameters used to configure kinematics plugins
 */
struct ARM_KINEMATICS_PUBLIC KinematicsParams {
  using NodeParametersInterface = rclcpp::node_interfaces::NodeParametersInterface;
  using NodeInterfaces = rclcpp::node_interfaces::NodeInterfaces<NodeParametersInterface>;
  using SharedPtr = std::shared_ptr<KinematicsParams>;

  /// The name of the link in the URDF to treat as the 'origin' for kinematics
  std::string base_link_name = "";

  /// The name of the link to use by default as the target in FK and IK calculations. Usually the end effector.
  std::string ee_link_name = "";

  std::vector<std::string> joint_names = {};

  /**
   * Constructor
   * @param node The parameter interface to get params from
   */
  explicit KinematicsParams(const NodeParametersInterface::SharedPtr & node);

  /**
   * Constructor
   * @param node The parameter interface to get params from
   */
  explicit KinematicsParams(NodeInterfaces node);

  /**
   * Default Constructor
   */
  explicit KinematicsParams() = default;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_KINEMATICS_PARAMS_HPP
