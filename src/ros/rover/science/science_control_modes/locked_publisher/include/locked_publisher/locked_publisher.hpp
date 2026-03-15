#ifndef LOCKED_PUBLISHER__LOCKED_PUBLISHER_HPP_
#define LOCKED_PUBLISHER__LOCKED_PUBLISHER_HPP_

#include <rclcpp/time.hpp>
#include <string>
#include "control_mode/control_mode.hpp"
#include "locked_publisher/visibility_control.h"
#include "nova_interfaces/msg/locked_status.hpp"

namespace locked_publisher
{

using namespace control_mode;

/**
 * Control Mode that publishes the locked status of Teleop Modular.
 */
class LOCKED_PUBLISHER_PUBLIC LockedPublisher : public ControlMode
{
public:
  LockedPublisher();

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
  ~LockedPublisher() override;

private:
  /// Helper struct to hold parameters used by the control mode.
  struct Params {
    /// The topic name to send messages to.
    std::string topic = "";

    /// The ROS2 topic Quality of Service depth value to use in publisher_.
    int qos_depth = 1;
    /// The ROS2 topic Quality of Service's reliability to use in publisher_. Uses 'reliable' when true.
    bool qos_reliable = false;
  };

  /// Stores current parameter values
  Params params_;

  rclcpp::Publisher<nova_interfaces::msg::LockedStatus>::SharedPtr publisher_;
};

}  // namespace locked_publisher

#endif  // LOCKED_PUBLISHER__LOCKED_PUBLISHER_HPP_
