//
// Created by nova on 6/4/25.
//

#ifndef CONTROLMODEMANAGER_HPP
#define CONTROLMODEMANAGER_HPP

#include <map>
#include <string>
#include <rclcpp/node.hpp>
#include <rclcpp/node_interfaces/node_base_interface.hpp>

#include "ControlMode.hpp"

namespace controller_manager_msgs::srv {
struct SwitchController;
}

namespace teleop_arm_joy {

/**
 * Class responsible for managing the registered control modes, the current control mode, and switching between them.
 */
class ControlModeManager {
public:
  explicit ControlModeManager(const std::shared_ptr<rclcpp::Node>& node) : node_(node) {
  }

  /**
   * Populates the control_modes_ from the params in node_.
   */
  void configure();

  /**
   * @brief Attempts to activate a control mode.
   * @param name The name of the control mode to load.
   * @return True if successfully switched to the control mode.
   */
  bool set_control_mode(const std::string& name);

private:
  /**
   * Resets everything for the controller manager
   */
  void reset();

  /**
   * Switches ros2_control controllers in the controller_manager for the given change in control modes.
   * @param previous the control mode being deactivated.
   * @param next the control mode being activated.
   * @return
   */
  [[nodiscard]] bool switch_controllers(const ControlMode& previous, const ControlMode& next) const;

  /**
   * The owning teleop_arm_joy ROS2 node
   */
  std::shared_ptr<rclcpp::Node> node_;

  std::map<std::string, std::shared_ptr<ControlMode>> control_modes_ = {};
  std::shared_ptr<ControlMode> current_control_mode_ = nullptr;

  // Service calls
  /// Client to call the service on the controller manager to change the currently active controllers.
  rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr switch_controller_client_ = nullptr;
};

}

#endif //CONTROLMODEMANAGER_HPP
