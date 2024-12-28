/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Auger controller, for controlling the big science
  drill.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: auger
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
  - subscriber: /inputs/input_joystick_r [InputJoystick]
SERVICES:
  - /service_name [Service Type]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auger_controller
AUTHOR(S):	Arbab Ahmed
CREATION:	04/12/2024
EDITED:		04/12/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Item One
 - Item Two
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#ifndef AUGER_CONTROLLER__AUGER_CONTROLLER_HPP_
#define AUGER_CONTROLLER__AUGER_CONTROLLER_HPP_

#include <cstdlib>
#include "auger_controller/visibility_control.h"
#include "controller_interface/controller_interface.hpp"
#include "input_interfaces/msg/input_joystick.hpp"
#include "realtime_tools/realtime_publisher.h"
#include "rclcpp/node.hpp"

#include "auger_controller_parameters.hpp"

namespace auger_controller
{

class AugerController : public controller_interface::ControllerInterface
{
public:
  AUGER_CONTROLLER_PUBLIC
  AugerController();

  AugerController(const AugerController &) = delete;
  AugerController(AugerController &&) = delete;
  AugerController &operator=(const AugerController &) = delete;
  AugerController &operator=(AugerController &&) = delete;
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

  bool reset();

  void stop_auger();
  void stop_drill();

  void deadline_callback(); // might not need this

  // why do we need this? commenting until i'm certain we don't need this
  //bool check_joystick_lock();
  void update_joystick_lock(input_interfaces::msg::InputJoystick joystick_l);
  
  void update_auger_height(input_interfaces::msg::InputJoystick joystick_r);
  void update_drill_spin(input_interfaces::msg::InputJoystick joystick_r);
  void joystick_l_callback(input_interfaces::msg::InputJoystick msg);
  void joystick_r_callback(input_interfaces::msg::InputJoystick msg);


protected:
  int auger_direction;
  int drill_direction;
  int auger_velocity;
  int drill_velocity;
  int auger_max_velocity;
  int drill_max_velocity;

  bool top_limit;
  bool bottom_limit;
  bool joystick_lock;
  bool subscriber_is_active_ = false;

  //struct AugerHandle
  //{
	// C++ likes throwing a fit if we don't initialise command, so we wrap it in this
	// struct so that it may leave us alone. We will initialise this before we use it.
    //std::reference_wrapper<const hardware_interface::LoanedStateInterface> state;
    //std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
  //};
  
  //AugerHandle handle;
  
  std::optional<std::reference_wrapper<hardware_interface::LoanedCommandInterface>> command;
 
  using InputJoystick = input_interfaces::msg::InputJoystick;
  std::shared_ptr<rclcpp::Publisher<InputJoystick>> joystick_publisher_ = nullptr;
  std::unique_ptr<realtime_tools::RealtimePublisher<InputJoystick>> realtime_joystick_publisher_ = nullptr;

  // Parameters from ROS for auger_controller
  // TODO: params
  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  rclcpp::Node node;

  rclcpp::Subscription<input_interfaces::msg::InputJoystick>::SharedPtr joystick_l_sub = nullptr;
  rclcpp::Subscription<input_interfaces::msg::InputJoystick>::SharedPtr joystick_r_sub = nullptr;

  // publish rate limiter
  // double publish_rate_ = 50.0;
  // rclcpp::Duration publish_period_ = rclcpp::Duration::from_nanoseconds(0);
  // rclcpp::Time previous_publish_timestamp_ {0, 0, RCL_CLOCK_UNINITIALIZED};
};

}  // namespace auger_controller

#endif  // AUGER_CONTROLLER__AUGER_CONTROLLER_HPP_
