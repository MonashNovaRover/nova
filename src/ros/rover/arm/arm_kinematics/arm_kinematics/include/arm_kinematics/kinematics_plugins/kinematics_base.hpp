//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_KINEMATICS_BASE_HPP
#define ARM_KINEMATICS_KINEMATICS_BASE_HPP

#include <arm_kinematics/visibility_control.h>
#include <string>
#include <rclcpp/logger.hpp>
#include <rclcpp/node_interfaces/node_base_interface.hpp>
#include <rclcpp/node_interfaces/node_logging_interface.hpp>
#include <rclcpp/node_interfaces/node_parameters_interface.hpp>
#include <rclcpp/node_interfaces/node_interfaces.hpp>
#include "kinematics_params.hpp"

namespace arm_kinematics {

/**
 * Base class for the two kinematics plugin types,
 *   - ForwardKinematicsPluginBase
 *   - InverseKinematicsPluginBase
 */
class ARM_KINEMATICS_PUBLIC KinematicsBase {
public:
  using KinematicsNodeInterfaces =
    rclcpp::node_interfaces::NodeInterfaces<
      rclcpp::node_interfaces::NodeBaseInterface,
      rclcpp::node_interfaces::NodeLoggingInterface,
      rclcpp::node_interfaces::NodeParametersInterface>;

  // Accessors

  /// The URDF being used.
  [[nodiscard]] const std::string & get_robot_description() const;
  /// The name of every joint considered in IK and FK calculations.
  [[nodiscard]] const std::vector<std::string> & get_joint_names() const noexcept;
  /// Logger to use for logging
  [[nodiscard]] const rclcpp::Logger & get_logger() const noexcept;
  /// Gets the KinematicsParams common to both FK and IK plugins.
  [[nodiscard]] const KinematicsParams & get_kinematics_params() const noexcept;
  /// Gets interfaces from the owning ROS2 node, allowing plugins to access to parameters, logging, etc.
  [[nodiscard]] const KinematicsNodeInterfaces & get_node_interfaces() const;

protected:
  /**
   * Initializer for common KinematicsBase base class.
   */
  bool initialize_base(KinematicsNodeInterfaces node_interfaces,
                       std::string & robot_description,
                       const std::vector<std::string> & joint_names,
                       KinematicsParams params,
                       const std::string & logger_name);

private:
  /// The URDF being used. You can get this from within a ros2_control controller.
  std::string * robot_description_ = nullptr;
  /// Params common to both FK and IK plugins.
  KinematicsParams kinematics_params_;
  /// The name of every joint considered in IK and FK calculations.
  std::vector<std::string> joint_names_{};
  /// Allows us to access various things from the owning node if we need, like loggers, parameters, or in the future,
  /// maybe even topics.
  std::optional<KinematicsNodeInterfaces> node_interfaces_ = std::nullopt;
  /// Logger to use for logging
  rclcpp::Logger logger_ = rclcpp::get_logger("arm_kinematics");
};

} // arm_kinematics

#endif //ARM_KINEMATICS_KINEMATICS_BASE_HPP
