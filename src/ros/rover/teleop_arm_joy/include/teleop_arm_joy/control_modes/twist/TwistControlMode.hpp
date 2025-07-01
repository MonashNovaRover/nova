//
// Created by Bailey Chessum on 6/4/25.
//

#ifndef TWISTIKCONTROLMODE_HPP
#define TWISTIKCONTROLMODE_HPP

#include "twist_control_mode_parameters.hpp"
#include "teleop_arm_joy/control_modes/ControlMode.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"

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
  Input<double>::SharedPtr speed_coefficient_;
  /// Do nothing when this is true
  Input<bool>::SharedPtr locked_;

  // Inputs for all the twist components
  Input<double>::SharedPtr x_;
  Input<double>::SharedPtr y_;
  Input<double>::SharedPtr z_;
  Input<double>::SharedPtr yaw_;
  Input<double>::SharedPtr pitch_;
  Input<double>::SharedPtr roll_;

  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr publisher_;
};

} // teleop_arm_joy

#endif //TWISTIKCONTROLMODE_HPP
