#ifndef AUGER_CONTROLLER__AUGER_CONTROLLER_HPP_
#define AUGER_CONTROLLER__AUGER_CONTROLLER_HPP_

#include "auger_controller/visibility_control.h"
#include "controller_interface/controller_interface.hpp"

namespace auger_controller
{

class AugerController : public controller_interface::ControllerInterface
{
public:
  AUGER_CONTROLLER_PUBLIC
  AugerController();

  virtual ~AugerController();

  AUGER_CONTROLLER_PUBLIC
  controller_interface::InterfaceConfiguration command_interface_configuration() const override;

  AUGER_CONTROLLER_PUBLIC
  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  AUGER_CONTROLLER_PUBLIC
  controller_interface::return_type update(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  AUGER_CONTROLLER_PUBLIC
  controller_interface::CallbackReturn on_init() override;

  AUGER_CONTROLLER_PUBLIC
  controller_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State & previous_state) override;

  AUGER_CONTROLLER_PUBLIC
  controller_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State & previous_state) override;

  AUGER_CONTROLLER_PUBLIC
  controller_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State & previous_state) override;

  AUGER_CONTROLLER_PUBLIC
  controller_interface::CallbackReturn on_cleanup(
    const rclcpp_lifecycle::State & previous_state) override;

  AUGER_CONTROLLER_PUBLIC
  controller_interface::CallbackReturn on_error(
    const rclcpp_lifecycle::State & previous_state) override;

  AUGER_CONTROLLER_PUBLIC
  controller_interface::CallbackReturn on_shutdown(
    const rclcpp_lifecycle::State & previous_state) override;

};

}  // namespace auger_controller

#endif  // AUGER_CONTROLLER__AUGER_CONTROLLER_HPP_
