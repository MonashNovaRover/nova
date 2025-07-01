//
// Created by Bailey Chessum on 4/6/25.
//

#ifndef INPUTSOURCE_HPP
#define INPUTSOURCE_HPP

#include <rclcpp/node.hpp>

#include "InputSourceUpdateDelegate.hpp"
#include "teleop_arm_joy/inputs/events/Event.hpp"
#include "teleop_arm_joy/inputs/InputCommon.hpp"
#include "teleop_arm_joy/inputs/InputManager.hpp"
#include "teleop_arm_joy/inputs/InputDeclaration.hpp"

namespace teleop_arm_joy
{
/**
 * A base class for various sources of inputs and event invokers, such as joysticks, keyboards, the GUI, etc.
 */
class InputSource {

public:
  virtual ~InputSource() = default;

  void initialize(const std::shared_ptr<rclcpp::Node>& node, const std::string& name, const std::weak_ptr<InputSourceUpdateDelegate>& delegate);
  void update(const rclcpp::Time& now);

  // Accessors
  [[nodiscard]] const std::string& get_name() const {
    return name_;
  }

  [[nodiscard]] std::shared_ptr<rclcpp::Node> get_node() const {
    return node_;
  }
  [[nodiscard]] std::shared_ptr<rclcpp::Node> get_node() {
    return node_;
  }

protected:
  /**
   * Called when starting up the input source, allowing the implementation to get configuration from it's node.
   */
  virtual void on_initialize() = 0;

  /**
   * @brief Called after on_configure(), allows the input source to declare memory pointing to button values.
   *
   * @attention Be very careful to make sure you don't make a copy adding to the declarations vector. Everything needs
   * to be a reference to a variable held by your class:
   * @code
   * // This will cause a segfault!!
   * auto button = buttons_.emplace_back(button_name, button_config);
   * declarations.emplace_back(button.name, button.value);
   *
   * // This is good, since we use a reference &
   * auto & button = buttons_.emplace_back(button_name, button_config);
   * declarations.emplace_back(button.name, button.value);
   * @endcode
   * If this causes a segfault, you've likely copied memory by accident.
   *
   * @param[out] declarations Information defining each button exposed by the input source, and a reference to the
   * memory holding the value for that input.
   */
  virtual void export_buttons(std::vector<InputDeclaration<bool>>& declarations) = 0;

  /**
   * @brief Called after on_configure(), allows the input source to declare memory pointing to axis values.
   *
   * @attention Be very careful to make sure you don't make a copy adding to the declarations vector. Everything needs
   * to be a reference to a variable held by your class:
   * @code
   * // This will cause a segfault!!
   * auto axis = this->axes_.emplace_back(axis_name, axis_config);
   * declarations.emplace_back(axis.name, axis.value);
   *
   * // This is good, since we use a reference &
   * auto& axis = this->axes_.emplace_back(axis_name, axis_config);
   * declarations.emplace_back(axis.name, axis.value);
   * @endcode
   * If this causes a segfault, you've likely copied memory by accident.
   *
   * @param[out] declarations Information defining each button exposed by the input source, and a reference to the
   * memory holding the value for that input.
   */
  virtual void export_axes(std::vector<InputDeclaration<double>>& declarations) = 0;

  /**
   * Call this whenever you receive a new input and you want the InputManager to invoke a new update.
   */
  void request_update(const rclcpp::Time& now = rclcpp::Time()) const;

  /**
   * Called when new input should be processed, after requesting an update through request_update().
   */
  virtual void on_update(const rclcpp::Time& now) {};

  /// The ROS2 node created by teleop_arm_joy, which we get params from (for base and child classes)
  std::shared_ptr<rclcpp::Node> node_ = nullptr;

private:
  std::string name_;
  std::weak_ptr<InputSourceUpdateDelegate> delegate_{};
};

}

#endif // INPUTSOURCE_HPP
