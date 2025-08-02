#include "legacy_input_mode/legacy_input_mode.hpp"

namespace legacy_input_mode
{

LegacyInputMode::LegacyInputMode() = default;

LegacyInputMode::~LegacyInputMode() = default;

return_type LegacyInputMode::on_init()
{
  auto node = get_node();

  // Do any initialization logic here!
  // This effectively replaces the constructor for anything that depends on get_node()

  // Declare parameters here! Or consider using something like generate_parameter_library instead.
  node->declare_parameter<std::string>("topic", "");
  node->declare_parameter<int>("qos", 10);

  return return_type::OK;
}

CallbackReturn LegacyInputMode::on_configure(const State &)
{
  auto node = get_node();
  const auto logger = get_node()->get_logger();

  // Use this callback method to get any parameters for your control mode!
  params_ = Params();
  node->get_parameter<std::string>("topic", params_.topic);
  node->get_parameter<int>("qos", params_.qos);

  // Create the publishers based on the params we just got
  if (params_.topic.empty()) {
    // You've probably made a mistake if the topic isn't set!
    RCLCPP_ERROR(logger, "The \"topic\" parameter must be set to a valid topic name!");
    return CallbackReturn::ERROR;
  }

  // TODO: Set an appropriate message type for the publisher, then uncomment its declaration/usages
  // publisher_ = get_node()->create_publisher<TODO>(params_.topic, params_.qos);

  return CallbackReturn::SUCCESS;
}

void LegacyInputMode::on_capture_inputs(Inputs inputs)
{
  // This method is always run after on_configure(),
  // so you can assume that you already have any necessary parameters

  // Capture inputs like this:
  speed_ = inputs.axes["speed"];

  // TODO: Add Axis::SharedPtr and/or Button::SharedPtr member variables, then assign them here.
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
  // TODO: Implement for your message type, or remove the method if it is not appropriate for the use case.
  // auto msg = std::make_unique<TODO>();
  // publisher_->publish(std::move(msg));
}

return_type LegacyInputMode::on_update(const rclcpp::Time & now, const rclcpp::Duration & period)
{
  const auto logger = get_node()->get_logger();

  // Don't move when locked
  if (is_locked()) {
    publish_halt_message(now);
    return return_type::OK;
  }

  // Get input values either with input_->value() or by referencing and implicitly casting *input_
  const float speed = std::max(speed_->value(), 0.0f);

  // TODO: Construct and send a message using values from inputs
  // auto msg = std::make_unique<TODO>();

  // msg->some_value = *some_axis_ * speed;
  // msg->header.stamp = now;

  // publisher_->publish(std::move(msg));

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
  speed_.reset();
  // publisher_.reset();

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
