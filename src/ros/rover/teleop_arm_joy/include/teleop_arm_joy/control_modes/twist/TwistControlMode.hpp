//
// Created by Bailey Chessum on 6/4/25.
//

#ifndef TWISTIKCONTROLMODE_HPP
#define TWISTIKCONTROLMODE_HPP

#include "twist_control_mode_parameters.hpp"
#include "teleop_arm_joy/control_modes/ControlMode.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "teleop_arm_joy/inputs/Button.hpp"
#include "teleop_arm_joy/inputs/Axis.hpp"

namespace teleop_arm_joy {
/**
 * Control mode for moving joint velocities directly
 */
class TwistControlMode final : public ControlMode {

public:
  explicit TwistControlMode() = default;

  void on_initialize() override;

  void on_configure(InputManager& inputs) override;
  void on_activate() override;
  void on_deactivate() override;

  void publish_halt_message(const rclcpp::Time& now) const;

  void update(const rclcpp::Time& now, const rclcpp::Duration& period) override;

protected:
  ~TwistControlMode() override = default;

  /// Helper function to get the euclidean length of a vector, used for normalized limits.
  double norm(double x, double y, double z);

  /// Tracks parameters
  std::shared_ptr<twist_control_mode::ParamListener> param_listener_{};
  twist_control_mode::Params params_{};

  /// Input from 0 to 1 that directly scales the output speed.
  Axis::SharedPtr speed_coefficient_;
  /// Do nothing when this is true
  Button::SharedPtr locked_;

  // Inputs for all the twist components
  Axis::SharedPtr x_;
  Axis::SharedPtr y_;
  Axis::SharedPtr z_;
  Axis::SharedPtr yaw_;
  Axis::SharedPtr pitch_;
  Axis::SharedPtr roll_;

  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr publisher_;
};

} // teleop_arm_joy

#endif //TWISTIKCONTROLMODE_HPP
