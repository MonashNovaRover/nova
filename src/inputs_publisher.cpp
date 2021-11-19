/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "inputs_publisher.h"


// Input function that publishes al of the inputs from the controllers
void InputsPublisher::publish_input () {

    // Updates the state of the gamepad controller
    GamepadUpdate();

    // Update the status of each controller
    gamepad->update();
    joystick_l->update();
    joystick_r->update();
    
    // Publish each of the data streams
    gamepad_publisher->publish(gamepad->get_message());
    joystick_l_publisher->publish(joystick_l->get_message());
    joystick_r_publisher->publish(joystick_r->get_message());
}


// Main consrtuctor sets up the node and the publishers
InputsPublisher::InputsPublisher() 
  : Node("input_pub"), count(0) {

    // Initialises the Gamepad inputs
    GamepadInit();

    // Creates all the joysticks
    gamepad = new JoystickGamepad(0.0);
    joystick_l = new JoystickThrustmaster(true, 0.0);
    joystick_r = new JoystickThrustmaster(false, 0.0);

    // Creates the publishers   
    gamepad_publisher = this->create_publisher<core::msg::InputGamepad>("/control/input_gamepad", 10);
    joystick_l_publisher = this->create_publisher<core::msg::InputJoystick>("/control/input_joystick_l", 10);
    joystick_r_publisher = this->create_publisher<core::msg::InputJoystick>("/control/input_joystick_r", 10);

    // Creates a timer function that runs a function on loop every 0.01 seconds
    timer = this->create_wall_timer(10ms, std::bind(&InputsPublisher::publish_input, this));
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<InputsPublisher>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}