#include "locked_publisher/locked_publisher.hpp"

namespace locked_publisher
{

LockedPublisher::LockedPublisher() = default;

LockedPublisher::~LockedPublisher() = default;

return_type LockedPublisher::on_init()
{
  auto node = get_node();

  // Do any initialization logic here!
  // This effectively replaces the constructor for anything that depends on get_node()

  // Declare parameters here! Or consider using something like generate_parameter_library instead.
  node->declare_parameter<std::string>("topic", "");
  node->declare_parameter<int>("qos.depth", 1);
  node->declare_parameter<bool>("qos.reliable", true);  //< TODO: Check your subscribers use best_effort before making
                                                        //        false, but its better for teleop to use best_effort
  return return_type::OK;
}

CallbackReturn LockedPublisher::on_configure(const State &)
{
  auto node = get_node();
  const auto logger = get_node()->get_logger();

  // Use this callback method to get any parameters for your control mode!
  params_ = Params();
  node->get_parameter<std::string>("topic", params_.topic);
  node->get_parameter<int>("qos.depth", params_.qos_depth);
  node->get_parameter<bool>("qos.reliable", params_.qos_reliable);

  // Create the publishers based on the params we just got
  if (params_.topic.empty()) {
    // You've probably made a mistake if the topic isn't set!
    RCLCPP_ERROR(logger, "The \"topic\" parameter must be set to a valid topic name!");
    return CallbackReturn::ERROR;
  }

  // Build QoS profile from params
  rclcpp::QoS qos_profile = rclcpp::QoS(rclcpp::KeepLast(params_.qos_depth));
  if (params_.qos_reliable)
    qos_profile.reliable();
  else
    qos_profile.best_effort();

  publisher_ = get_node()->create_publisher<nova_interfaces::msg::LockedStatus>(params_.topic, qos_profile);

  RCLCPP_INFO(logger, "Locked Publisher configured, publishing to %s", params_.topic);

  return CallbackReturn::SUCCESS;
}

void LockedPublisher::on_configure_inputs(Inputs inputs)
{
}

CallbackReturn LockedPublisher::on_activate(const State &)
{
  return CallbackReturn::SUCCESS;
}

CallbackReturn LockedPublisher::on_deactivate(const State &)
{
  publish_halt_message(get_node()->now());
  return CallbackReturn::SUCCESS;
}

void LockedPublisher::publish_halt_message(const rclcpp::Time & now) const
{
  // System is locked, publish locked = true message
  auto msg = std::make_unique<nova_interfaces::msg::LockedStatus>();
  msg->header.stamp = now;
  msg->locked = true;
  publisher_->publish(std::move(msg));
}

return_type LockedPublisher::on_update(const rclcpp::Time & now, const rclcpp::Duration & period)
{
  const auto logger = get_node()->get_logger();

  // Don't move when locked
  if (is_locked()) {
    publish_halt_message(now);
    return return_type::OK;
  }

 // System is unlocked, publish locked = false message
  auto msg = std::make_unique<nova_interfaces::msg::LockedStatus>();
  msg->header.stamp = now;
  msg->locked = false;
  publisher_->publish(std::move(msg));

  return return_type::OK;
}

CallbackReturn LockedPublisher::on_error(const State &)
{
  // Called when any callback function returns CallbackReturn::ERROR

  return CallbackReturn::SUCCESS;
}

CallbackReturn LockedPublisher::on_cleanup(const State &)
{
  // Clear all state and return the control mode to a functionally equivalent state as after on_init() was first called.

  // Reset any held shared pointers
  publisher_.reset();

  params_ = Params();

  return CallbackReturn::SUCCESS;
}

CallbackReturn LockedPublisher::on_shutdown(const State &)
{
  // Clean up anything from on_init()

  return CallbackReturn::SUCCESS;
}

}  // namespace locked_publisher

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(locked_publisher::LockedPublisher, control_mode::ControlMode);
