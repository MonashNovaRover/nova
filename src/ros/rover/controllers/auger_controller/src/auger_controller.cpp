/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	auger_controller
AUTHOR(S):	Arbab Ahmed
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "auger_controller/auger_controller.hpp"
#include "lifecycle_msgs/msg/state.hpp"

using std::placeholders::_1;

namespace
{
  /*
   * constants
   */
  // can bus
  constexpr char CAN_BUS[] = "can1";
  // card IDs
  constexpr int AUGER_ID = 0x063;
  constexpr int DRILL_ID = 0x053;
  constexpr int CARD_ID_RECEIVE = 0x4A2;
  // command data
  constexpr int AUGER_UP = 1;
  constexpr int AUGER_DOWN = -1;
  constexpr int DRILL_CLOCKWISE = 1;
  constexpr int DRILL_COUNTERCLOCKWISE = -1;
  // limit switch id
  constexpr int AUGER_LIMIT_SWITCH_TOP = 0x01;
  constexpr int AUGER_LIMIT_SWITCH_BOTTOM = 0x02;
  // limit switch status/data
  constexpr int AUGER_LIMIT_SWITCH_CLEAR = 0x00;
  constexpr int AUGER_LIMIT_SWITCH_HIT = 0xFF;
  // max velocity
  constexpr float MAX_VELOCITY = 32767.f * 3/4; // 3/4 of max possible value sent to motor
  // ROS parameter names
  constexpr char CAN_BUS_PARAM[] = "can_bus";
  constexpr char AUGER_MAX_VELOCITY_PARAM[] = "auger_max_vel";
  constexpr char DRILL_MAX_VELOCITY_PARAM[] = "drill_max_vel";

  constexpr char DEFAULT_JOYSTICK_TOPIC[] = "~/joystick";
}
namespace auger_controller
{

  AugerController::AugerController() : node("auger")
  {
  }

  AugerController::~AugerController()
  {
  }
  
  controller_interface::InterfaceConfiguration AugerController::command_interface_configuration() const
  {
      std::vector<std::string> conf_names;
      conf_names.emplace_back("auger/effort");

      return {controller_interface::interface_configuration_type::INDIVIDUAL, conf_names};
  }

  controller_interface::InterfaceConfiguration AugerController::state_interface_configuration() const
  {
      std::vector<std::string> conf_names;

      return {controller_interface::interface_configuration_type::INDIVIDUAL, conf_names};
  }

  controller_interface::return_type AugerController::update(const rclcpp::Time &time, const rclcpp::Duration &period)
  {
	  auto logger = get_node()->get_logger();
	  if (get_lifecycle_state().id() == lifecycle_msgs::msg::State::PRIMARY_STATE_INACTIVE)
	  {
	  	  return controller_interface::return_type::OK;
	  }

	  // update parameters if they have changed
	  if (param_listener_->is_old(params_))
	  {
		  params_ = param_listener_->get_params();
		  RCLCPP_INFO(logger, "Parameters were updated");
	  }

	  const double feedback = command.value().get().get_value();
	  if (std::isnan(feedback))
	  {
		  RCLCPP_ERROR(logger, "Auger command is invalid.");
		  return controller_interface::return_type::ERROR;
	  }
	  
	  if (realtime_joystick_publisher_->trylock())
	  {
		  auto& msg = realtime_joystick_publisher_->msg_;
		  
		  msg.set__ax_slider(feedback);

		  realtime_joystick_publisher_->unlockAndPublish();
	  }

	  return controller_interface::return_type::OK;
  }

  controller_interface::CallbackReturn AugerController::on_init()
  {
	controller_interface::ControllerInterface::on_init();

	try 
	{
		// create the parameter listener and get the parameters
		param_listener_ = std::make_shared<ParamListener>(get_node());
		params_ = param_listener_->get_params();
	}
	catch (const std::exception &e) 
	{
		fprintf(stderr, "Exception thrown during init state with message: %s", e.what());
		return CallbackReturn::ERROR;
	}

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
	  auto logger = get_node()->get_logger();
	  
	  // update parameters if they have changed
	  if (param_listener_->is_old(params_))
	  {
		  params_ = param_listener_->get_params();
		  RCLCPP_INFO(logger, "Parameters were updated");
	  }

	  if (!reset())
	  {
		 return CallbackReturn::ERROR; 
	  }

	  // initialise publisher and message
	  joystick_publisher_ = get_node()->create_publisher<InputJoystick>(DEFAULT_JOYSTICK_TOPIC, rclcpp::SystemDefaultsQoS());
	  realtime_joystick_publisher_ = std::make_unique<realtime_tools::RealtimePublisher<InputJoystick>>(joystick_publisher_);

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
	if (command_interfaces_.empty())
	{
	    RCLCPP_ERROR(get_node()->get_logger(), "Unable to find command interface");
	    return controller_interface::CallbackReturn::ERROR;
	}

	// TODO: error checking
	command = command_interfaces_[0];

	subscriber_is_active_ = true;

	RCLCPP_DEBUG(get_node()->get_logger(), "Subscriber and publisher are active.");

	return controller_interface::CallbackReturn::SUCCESS;
  }


  controller_interface::CallbackReturn AugerController::on_deactivate(const rclcpp_lifecycle::State &previous_state)
  {
	subscriber_is_active_ = false;

	command.reset();
	
	return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn AugerController::on_cleanup(const rclcpp_lifecycle::State &previous_state)
  {
	// set received ros2 topic messages to defaults
	
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

  bool AugerController::reset()
  {
	command.reset();

	return true;
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
