//
// Created by nova on 6/4/25.
//

#ifndef CONTROLMODEMANAGER_HPP
#define CONTROLMODEMANAGER_HPP

#include <map>
#include <string>
#include <rclcpp/node.hpp>
#include <controller_manager_msgs/srv/switch_controller.hpp>
#include <pluginlib/class_loader.hpp>
#include <rclcpp/executor.hpp>

#include "ControlMode.hpp"
#include "teleop_arm_joy/inputs/InputManager.hpp"

namespace teleop_arm_joy {

/**
 * Class responsible for managing the registered control modes, the current control mode, and switching between them.
 */
class ControlModeManager final : public Collection<ControlMode> {
public:
  explicit ControlModeManager(const std::shared_ptr<rclcpp::Node>& node, const std::weak_ptr<rclcpp::Executor>& executor)
    : node_(node), executor_(executor) {}
  ~ControlModeManager() override;

  /**
   * Populates the control_modes_ from the params in node_.
   */
  void configure(InputManager& inputs);

  /**
   * @brief Attempts to activate a control mode.
   * @param name The name of the control mode to load.
   * @return True if successfully switched to the control mode.
   */
  bool set_control_mode(const std::string& name);

  /**
   * Update the active node
   */
  auto update(const rclcpp::Time& now, const rclcpp::Duration& period) const -> void;

  // Collection<ControlMode> implementation
  std::shared_ptr<ControlMode> operator[](const std::string& index) override;

  iterator begin() override { return control_modes_.begin(); };
  [[nodiscard]] const_iterator begin() const override { return control_modes_.begin(); };
  iterator end() override { return control_modes_.end(); };
  [[nodiscard]] const_iterator end() const override { return control_modes_.end(); };

  void add(const std::string& key, const std::shared_ptr<ControlMode>& value) override;

private:
  /**
   * Resets everything for the controller manager
   */
  void reset();

  /**
   * Switches ros2_control controllers in the controller_manager for the given change in control modes.
   * @param previous the control mode being deactivated.
   * @param next the control mode being activated.
   * @return True if the request was made successfully. False otherwise.
   */
  [[nodiscard]] bool switch_controllers(const ControlMode& previous, const ControlMode& next) const;

  /**
   * Switches ros2_control controllers in the controller_manager for the given change in control modes.
   * @param next the control mode being activated.
   * @return True if the request was made successfully. False otherwise.
   */
  [[nodiscard]] bool switch_controllers(const ControlMode& next) const;

  /**
   * Gets the control mode plugin class type name for a given control mode name, to be given to pluginlib to load.
   * Declares the necessary parameter to get the type name as a side effect.
   * @param[in]  name The name of the control mode to get the plugin type name for.
   * @param[out] control_mode_type The output control mode plugin type name to be given to pluginlib.
   * @return True if the type name was found. False otherwise.
   */
  bool get_type_for_control_mode(const std::string& name, std::string& control_mode_type) const;

  /// The owning teleop_arm_joy ROS2 node.
  std::shared_ptr<rclcpp::Node> node_;
  /// Add spawned nodes to this to get them to spin
  std::weak_ptr<rclcpp::Executor> executor_;

  // Control modes
  /// Loads the control modes, and needs to stay alive during the whole lifecycle of the control modes.
  std::unique_ptr<pluginlib::ClassLoader<ControlMode>> control_mode_loader_;

  /// Currently loaded control modes.
  std::map<std::string, std::shared_ptr<ControlMode>> control_modes_{};
  /// The currently active control mode.
  std::shared_ptr<ControlMode> current_control_mode_ = nullptr;

  // Service calls
  /// Client to call the service on the controller manager to change the currently active controllers.
  rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr switch_controller_client_ = nullptr;
};

}

#endif //CONTROLMODEMANAGER_HPP
