/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	auger_controller
AUTHOR(S):	Arbab Ahmed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "auger_controller/auger_controller.hpp"
#include <string>

using std::placeholders::_1;
namespace auger_controller
{

  AugerController::AugerController() : node("auger")
  {
  }

  AugerController::~AugerController()
  {
  }

  controller_interface::return_type AugerController::update(const rclcpp::Time &time, const rclcpp::Duration &period)
  {

  }

  controller_interface::CallbackReturn AugerController::on_init()
  {
	controller_interface::ControllerInterface::on_init();

	node.declare_parameter(CAN_BUS_PARAM, CAN_BUS);
	node.declare_parameter(AUGER_MAX_VELOCITY_PARAM, MAX_VELOCITY);
	node.declare_parameter(DRILL_MAX_VELOCITY_PARAM, MAX_VELOCITY);

	// initially all motors spin backwards with 0 velocity
	auger_direction = AUGER_UP;
	drill_direction = DRILL_CLOCKWISE;
	auger_velocity = 0;
	drill_velocity = 0;

	top_limit = false;
	bottom_limit = false;
	joystick_lock = true;

	return CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn AugerController::on_configure(const rclcpp_lifecycle::State &previous_state)
  {
	auger_max_velocity = node.get_parameter(AUGER_MAX_VELOCITY_PARAM).as_int();
	drill_max_velocity = node.get_parameter(DRILL_MAX_VELOCITY_PARAM).as_int();

	joystick_l_sub = node.create_subscription<input_interfaces::msg::InputJoystick>("/inputs/input_joystick_l", 
			rclcpp::SystemDefaultsQoS(), std::bind(&AugerController::joystick_l_callback, this, _1));
	joystick_r_sub = node.create_subscription<input_interfaces::msg::InputJoystick>("/inputs/input_joystick_r", 
			rclcpp::SystemDefaultsQoS(), std::bind(&AugerController::joystick_r_callback, this, _1));

	return CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn AugerController::on_activate(const rclcpp_lifecycle::State &previous_state)
  {
	return controller_interface::CallbackReturn::SUCCESS;
  }


  controller_interface::CallbackReturn AugerController::on_deactivate(const rclcpp_lifecycle::State &previous_state)
  {
	return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn AugerController::on_cleanup(const rclcpp_lifecycle::State &previous_state)
  {
	return controller_interface::CallbackReturn::SUCCESS;
  }


  controller_interface::CallbackReturn AugerController::on_error(const rclcpp_lifecycle::State &previous_state)
  {
	return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn AugerController::on_shutdown(const rclcpp_lifecycle::State &previous_state)
  {
	return controller_interface::CallbackReturn::SUCCESS;
  }

  void AugerController::stop_auger()
  {
	auger_velocity = 0;
	auger_direction = AUGER_UP;
  }

  void AugerController::stop_drill()
  {
	drill_velocity = 0;
	drill_direction = DRILL_CLOCKWISE;
  }

  /*bool AugerController::check_joystick_lock()
  {
	if (joystick_lock)
	{
	    stop_auger();
	    stop_drill();
	    return true;
	}
	return false;
  }*/

  void AugerController::update_joystick_lock(input_interfaces::msg::InputJoystick joystick_l)
  {
	// joysticks lock if bottom L2 button is pressed on the left joystick
	if (joystick_l.btn_bottom_l2_state >= 1 and !joystick_lock)
	{
	    // todo: log joystick on
	    joystick_lock = true;
	    stop_auger();
	    stop_drill();
	}

	// joysticks unlock if bottom L5 button is pressed on the left joystick
	if (joystick_l.btn_bottom_l5_state >= 1 and joystick_lock)
	{
	    // todo: log joystick off
	    joystick_lock = false;
	}
  }

  void AugerController::update_auger_height(input_interfaces::msg::InputJoystick joystick_r)
  {
	// auger height direction is determined by the right joystick's x-axis direction
	auger_direction = joystick_r.ax_stick_x >= 0 ? AUGER_DOWN : AUGER_UP;

	// auger velocity is determined by the right joystick's x-axis magnitude
	// if the auger is at the top or bottom limit, the velocity is set to 0
	if ((auger_direction == AUGER_UP && top_limit)
	    || (auger_direction == AUGER_DOWN && bottom_limit))
	{
	    auger_velocity = 0;
	}
	else
	{
	    auger_velocity = abs(auger_max_velocity * joystick_r.ax_stick_x);
	}
  }

  void AugerController::update_drill_spin(input_interfaces::msg::InputJoystick joystick_r)
  {
	// drill spin direction is determined by the right joystick thumb buttons
	// thumb right = clockwise, thumb left = counterclockwise
	if (joystick_r.btn_thumb_r_state >= 1)
	    drill_direction = DRILL_CLOCKWISE;
	if (joystick_r.btn_thumb_l_state >= 1)
	    drill_direction = DRILL_COUNTERCLOCKWISE;
	
	// might not be needed anymore
	drill_velocity = joystick_r.btn_thumb_u_state >= 1 ? drill_max_velocity : 0;
  }

  void AugerController::joystick_l_callback(input_interfaces::msg::InputJoystick msg)
  {
	// logger call here
	update_joystick_lock(msg);
  }

  void AugerController::joystick_r_callback(input_interfaces::msg::InputJoystick msg)
  {
	// logger call here
	
	if (joystick_lock)
	    return;

	// update the inputs
	update_auger_height(msg);
	update_drill_spin(msg);
  }

}  // namespace auger_controller
