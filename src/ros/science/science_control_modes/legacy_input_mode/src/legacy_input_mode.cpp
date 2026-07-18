#include "legacy_input_mode/legacy_input_mode.hpp"

namespace legacy_input_mode
{

LegacyInputMode::LegacyInputMode() = default;

LegacyInputMode::~LegacyInputMode() = default;

int8_t LegacyInputMode::update_button(const uint8_t last_button, Button current_input)
{
  if (!current_input)
  {
    return 0;
  }

  // button currently not pressed
  if (current_input.value() == 0)
  {
    // nothing happening
    if (last_button == static_cast<uint8_t>(ButtonState::NOTHING) || last_button == static_cast<uint8_t>(ButtonState::UP))
    {
      return static_cast<uint8_t>(ButtonState::NOTHING);
    }
    // button unpressed
    return static_cast<uint8_t>(ButtonState::UP);
  }

  // button currently being pressed
  switch (last_button)
  {
    case static_cast<uint8_t>(ButtonState::NOTHING):
      return static_cast<uint8_t>(ButtonState::DOWN);
    case static_cast<uint8_t>(ButtonState::DOWN):
    case static_cast<uint8_t>(ButtonState::HELD):
      return static_cast<uint8_t>(ButtonState::HELD);
    case static_cast<uint8_t>(ButtonState::UP):
      return static_cast<uint8_t>(ButtonState::DOWN);
    default:
      assert(false);
  }
}

return_type LegacyInputMode::on_init()
{
  auto node = get_node();

  // Do any initialization logic here!
  // This effectively replaces the constructor for anything that depends on get_node()

  // Declare parameters here! Or consider using something like generate_parameter_library instead.
  node->declare_parameter<std::string>("topic", "");

  return return_type::OK;
}

CallbackReturn LegacyInputMode::on_configure(const State &)
{
  auto node = get_node();
  const auto logger = get_node()->get_logger();

  // Use this callback method to get any parameters for your control mode!
  params_ = Params();
  node->get_parameter<std::string>("topic", params_.topic);

  // Create the publishers based on the params we just got
  if (params_.topic.empty()) {
    // You've probably made a mistake if the topic isn't set!
    RCLCPP_ERROR(logger, "The \"topic\" parameter is empty, but must be set to a valid topic name!");
    return CallbackReturn::ERROR;
  }

  // This QOS is for legacy inputs don't do it like this.
  const std::chrono::milliseconds inputs_deadline {200};
  publisher_ = get_node()->create_publisher<input_interfaces::msg::InputJoystick>(params_.topic, rclcpp::QoS(1).best_effort().deadline(inputs_deadline));

  return CallbackReturn::SUCCESS;
}

void LegacyInputMode::on_configure_inputs(Inputs inputs)
{
  // This method is always run after on_configure(),
  // so you can assume that you already have any necessary parameters

  // Axis names
  const std::vector<std::string> axes_names = std::vector<std::string>{
    "ax_stick_x",
    "ax_stick_y",
    "ax_stick_twist",
    "ax_thumb_x",
    "ax_thumb_y",
    "ax_slider",
  };

  // Button names
  const std::vector<std::string> button_names = std::vector<std::string>{
    "btn_thumb_l_state",    // Left
    "btn_thumb_r_state",    // Right
    "btn_thumb_u_state",    // Up (behind the thumbstick)
    "btn_thumb_d_state",    // Down
    "btn_bottom_l1_state",
    "btn_bottom_l2_state",
    "btn_bottom_l3_state",
    "btn_bottom_l4_state",
    "btn_bottom_l5_state",
    "btn_bottom_l6_state",
    "btn_bottom_r1_state",
    "btn_bottom_r2_state",
    "btn_bottom_r3_state",
    "btn_bottom_r4_state",
    "btn_bottom_r5_state",
    "btn_bottom_r6_state",
  };

  // store shared pointers for axes
  for (auto& axis : axes_names)
  {
    axes_[axis] = inputs.axes[axis];
  }

  // store shared pointers for buttons
  for (auto& button : button_names)
  {
    buttons_[button] = inputs.buttons[button];
  }
}

CallbackReturn LegacyInputMode::on_activate(const State &)
{
  return CallbackReturn::SUCCESS;
}

CallbackReturn LegacyInputMode::on_deactivate(const State &)
{
  publish_halt_message(get_node()->now());
  return CallbackReturn::SUCCESS;
}

void LegacyInputMode::publish_halt_message(const rclcpp::Time & now) const
{
  auto msg = std::make_unique<input_interfaces::msg::InputJoystick>();
  publisher_->publish(std::move(msg));
}

return_type LegacyInputMode::on_update(const rclcpp::Time & now, const rclcpp::Duration & period)
{
  const auto logger = get_node()->get_logger();

  // Don't move when locked
  if (is_locked()) {
    publish_halt_message(now);
    return return_type::OK;
  }

  /// Update Joystick Msg
  // Buttons located near the thumb stick
  last_message_.btn_thumb_l_state = update_button(last_message_.btn_thumb_l_state, buttons_["btn_thumb_l_state"]);
  last_message_.btn_thumb_r_state = update_button(last_message_.btn_thumb_r_state, buttons_["btn_thumb_r_state"]);
  last_message_.btn_thumb_u_state = update_button(last_message_.btn_thumb_u_state, buttons_["btn_thumb_u_state"]);
  last_message_.btn_thumb_d_state = update_button(last_message_.btn_thumb_d_state, buttons_["btn_thumb_d_state"]);

  // Buttons located at the bottom left
  last_message_.btn_bottom_l1_state = update_button(last_message_.btn_bottom_l1_state, buttons_["btn_bottom_l1_state"]);
  last_message_.btn_bottom_l2_state = update_button(last_message_.btn_bottom_l2_state, buttons_["btn_bottom_l2_state"]);
  last_message_.btn_bottom_l3_state = update_button(last_message_.btn_bottom_l3_state, buttons_["btn_bottom_l3_state"]);
  last_message_.btn_bottom_l4_state = update_button(last_message_.btn_bottom_l4_state, buttons_["btn_bottom_l4_state"]);
  last_message_.btn_bottom_l5_state = update_button(last_message_.btn_bottom_l5_state, buttons_["btn_bottom_l5_state"]);
  last_message_.btn_bottom_l6_state = update_button(last_message_.btn_bottom_l6_state, buttons_["btn_bottom_l6_state"]);

  // Buttons located at the bottom right
  last_message_.btn_bottom_r1_state = update_button(last_message_.btn_bottom_r1_state, buttons_["btn_bottom_r1_state"]);
  last_message_.btn_bottom_r2_state = update_button(last_message_.btn_bottom_r2_state, buttons_["btn_bottom_r2_state"]);
  last_message_.btn_bottom_r3_state = update_button(last_message_.btn_bottom_r3_state, buttons_["btn_bottom_r3_state"]);
  last_message_.btn_bottom_r4_state = update_button(last_message_.btn_bottom_r4_state, buttons_["btn_bottom_r4_state"]);
  last_message_.btn_bottom_r5_state = update_button(last_message_.btn_bottom_r5_state, buttons_["btn_bottom_r5_state"]);
  last_message_.btn_bottom_r6_state = update_button(last_message_.btn_bottom_r6_state, buttons_["btn_bottom_r6_state"]);

  // Main Joystick Axis Data
  last_message_.ax_stick_x = axes_["ax_stick_x"].value();
  last_message_.ax_stick_y = axes_["ax_stick_y"].value();
  last_message_.ax_stick_twist = axes_["ax_stick_twist"].value();

  // Thumb Axis Data
  last_message_.ax_thumb_x = axes_["ax_thumb_x"].value();
  last_message_.ax_thumb_y = axes_["ax_thumb_y"].value();

  // Slider Data
  last_message_.ax_slider = axes_["ax_slider"].value();

  publisher_->publish(last_message_);

  return return_type::OK;
}

CallbackReturn LegacyInputMode::on_error(const State &)
{
  // Called when any callback function returns CallbackReturn::ERROR

  return CallbackReturn::SUCCESS;
}

CallbackReturn LegacyInputMode::on_cleanup(const State &)
{
  // Clear all state and return the control mode to a functionally equivalent state as after on_init() was first called.

  // Reset any held shared pointers
  for (auto& [fst, button] : buttons_)
  {
    button.reset();
  }
  for (auto& [fst, axis] : axes_)
  {
    axis.reset();
  }
  publisher_.reset();

  params_ = Params();

  return CallbackReturn::SUCCESS;
}

CallbackReturn LegacyInputMode::on_shutdown(const State &)
{
  // Clean up anything from on_init()

  return CallbackReturn::SUCCESS;
}

}  // namespace legacy_input_mode

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(legacy_input_mode::LegacyInputMode, control_mode::ControlMode);
