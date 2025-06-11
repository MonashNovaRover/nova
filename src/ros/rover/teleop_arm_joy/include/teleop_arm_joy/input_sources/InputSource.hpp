//
// Created by Bailey Chessum on 4/6/25.
//

#ifndef INPUTSOURCE_HPP
#define INPUTSOURCE_HPP

#include <rclcpp/node.hpp>
#include "teleop_arm_joy/inputs/Collection.hpp"
#include "teleop_arm_joy/inputs/Event.hpp"
#include "teleop_arm_joy/inputs/Input.hpp"
#include "teleop_arm_joy/inputs/InputManager.hpp"

namespace teleop_arm_joy
{
/**
 * A base class for various sources of inputs and event invokers, such as joysticks, keyboards, the GUI, etc.
 */
class InputSource {

public:
  virtual ~InputSource() = default;

  void initialize(const std::shared_ptr<rclcpp::Node>& node, const std::string& name);
  void configure(InputManager& inputs);

protected:
  virtual void on_initialize() {};
  virtual void on_configure(InputManager& inputs) {};

  /// The ROS2 node created by teleop_arm_joy, which we get params from (for base and child classes)
  std::shared_ptr<rclcpp::Node> node_ = nullptr;

private:
  std::string name_;
};

}

#endif // INPUTSOURCE_HPP
