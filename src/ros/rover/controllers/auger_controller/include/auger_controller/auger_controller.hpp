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
#include "rclcpp/node.hpp"
#include "rclcpp/qos.hpp"
#include "rclcpp/rclcpp.hpp"

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

  bool top_limit;
  bool bottom_limit;
  bool joystick_lock;

  rclcpp::Node node;

  rclcpp::Subscription<input_interfaces::msg::InputJoystick>::SharedPtr joystick_l_sub = nullptr;
  rclcpp::Subscription<input_interfaces::msg::InputJoystick>::SharedPtr joystick_r_sub = nullptr;

private:
  /*
   * constants
   */
  // can bus
  static constexpr char CAN_BUS[] = "can1";
  // card IDs
  static constexpr int AUGER_ID = 0x063;
  static constexpr int DRILL_ID = 0x053;
  static constexpr int CARD_ID_RECEIVE = 0x4A2;
  // command data
  static constexpr int AUGER_UP = 1;
  static constexpr int AUGER_DOWN = -1;
  static constexpr int DRILL_CLOCKWISE = 1;
  static constexpr int DRILL_COUNTERCLOCKWISE = -1;
  // limit switch id
  static constexpr int AUGER_LIMIT_SWITCH_TOP = 0x01;
  static constexpr int AUGER_LIMIT_SWITCH_BOTTOM = 0x02;
  // limit switch status/data
  static constexpr int AUGER_LIMIT_SWITCH_CLEAR = 0x00;
  static constexpr int AUGER_LIMIT_SWITCH_HIT = 0xFF;
  // max velocity
  static constexpr float MAX_VELOCITY = 32767.f * 3/4; // 3/4 of max possible value sent to motor
  // ROS parameter names
  static constexpr char CAN_BUS_PARAM[] = "can_bus";
  static constexpr char AUGER_MAX_VELOCITY_PARAM[] = "auger_max_vel";
  static constexpr char DRILL_MAX_VELOCITY_PARAM[] = "drill_max_vel";
};

}  // namespace auger_controller

#endif  // AUGER_CONTROLLER__AUGER_CONTROLLER_HPP_
