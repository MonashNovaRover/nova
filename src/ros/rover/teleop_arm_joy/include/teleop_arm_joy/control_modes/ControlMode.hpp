//
// Created by nova on 6/4/25.
//

#ifndef CONTROLMODE_HPP
#define CONTROLMODE_HPP

#include <memory>
#include <string>
#include <vector>
#include <rclcpp/node_interfaces/node_base_interface.hpp>

namespace teleop_arm_joy {
/**
 * Base class for a control mode used in teleoperation
 */
class ControlMode {
public:
  enum State {
    INACTIVE = 0,
    CONFIGURING = 1,
    ACTIVE = 2
  };

  /**
   * Params common to all ControlModes
   */
  struct Params {
    std::vector<std::string> controllers;
  };

  explicit ControlMode(const std::shared_ptr<rclcpp::node_interfaces::NodeBaseInterface>& node): node_(node) {

  }

  // Lifecycle methods

  void configure();
  void activate();
  void deactivate();

  virtual void update() {};

  // Accessors

  /// Name of the control mode, which the control mode is indexed by
  [[nodiscard]] const std::string& get_name() const {
    return name_;
  }

protected:
  ~ControlMode() = default;

  // Lifecycle methods
  virtual void on_configure(/* TODO: Give node, params, and any necessary context to set itself up */) {};
  virtual void on_activate() {};
  virtual void on_deactivate() {};

  /// The ROS2 node created by teleop_arm_joy, which we get params from (for base and child classes)
  std::shared_ptr<rclcpp::node_interfaces::NodeBaseInterface> node_;

  /// Params, from the base ControlMode type, populated by the ControlMode base class.
  Params base_params_;

public:
  [[nodiscard]] const Params& get_base_params() const {
    return base_params_;
  }

private:
  /// Name of the control mode, which the control mode is indexed by
  std::string name_;

};

} // teleop_arm_joy

#endif //CONTROLMODE_HPP
