#ifndef LEGACY_INPUT_MODE__LEGACY_INPUT_MODE_HPP_
#define LEGACY_INPUT_MODE__LEGACY_INPUT_MODE_HPP_

#include <rclcpp/time.hpp>
#include <string>
#include <chrono>
#include "control_mode/control_mode.hpp"
#include "legacy_input_mode/visibility_control.h"

#include "input_interfaces/msg/input_joystick.hpp"

namespace legacy_input_mode
{

using namespace control_mode;

/**
 * \class Publishes old input types for new teleop.
 */
class LEGACY_INPUT_MODE_PUBLIC LegacyInputMode : public ControlMode
{
public:
  enum class ButtonState:uint8_t
  {
    NOTHING = 0,
    DOWN = 1,
    HELD = 2,
    UP = 3,
  };

  LegacyInputMode();

  return_type on_init() override;
  CallbackReturn on_configure(const State & previous_state) override;
  void on_configure_inputs(Inputs inputs) override;

  /**
   * \brief Publishes a message to tell the control system to do nothing. Used when the control mode is locked, and
   * called once when deactivated.
   *
   * \param[in] now The time to associate with the 'halt' message.
   */
  void publish_halt_message(const rclcpp::Time & now) const;

  CallbackReturn on_activate(const State & previous_state) override;
  return_type on_update(const rclcpp::Time & now, const rclcpp::Duration & period) override;

  CallbackReturn on_deactivate(const State & previous_state) override;
  CallbackReturn on_cleanup(const State & previous_state) override;
  CallbackReturn on_error(const State & previous_state) override;
  CallbackReturn on_shutdown(const State & previous_state) override;

protected:
  ~LegacyInputMode() override;

private:
  /// Helper struct to hold parameters used by the control mode.
  struct Params {
    /// The topic name to send messages to.
    std::string topic = "";
  };

  /// Update Buttons
  static int8_t update_button(uint8_t last_button, Button::SharedPtr current_input);

  /// Stores current parameter values
  Params params_;

  // Create publisher
  rclcpp::Publisher<input_interfaces::msg::InputJoystick>::SharedPtr publisher_;

  // Store shared pointer in a map
  std::map<std::string, Button::SharedPtr> buttons_{};
  std::map<std::string, Axis::SharedPtr> axes_{};

  // Last message
  input_interfaces::msg::InputJoystick last_message_{};
};

}  // namespace legacy_input_mode

#endif  // LEGACY_INPUT_MODE__LEGACY_INPUT_MODE_HPP_
