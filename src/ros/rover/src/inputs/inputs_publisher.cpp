/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "inputs_publisher.h"
#include <print/print.h>


// Main consrtuctor sets up the node and the publishers
InputsPublisher::InputsPublisher() 
    : Node("input_pub"), count(0) {

    // Initialises the Gamepad inputs
    // Pass in the Joystick Identifier. You can find this by:
    //  cat /sys/class/input/js2/device/id/version on the correct js2 (input device)
    //  and then use VENDOR:PRODUCT
    GamepadInit("044F:B10A");

    // Creates all the joysticks (gamepads and thrustmasters)
    gamepad     = new JoystickGamepad(0.0);
    joystick_l  = new JoystickThrustmaster(true, 0.06445);
    joystick_r  = new JoystickThrustmaster(false, 0.06445);
    
    //Publisher options for QoS settings
    publisher_options.event_callbacks.deadline_callback = [](rclcpp::QOSDeadlineOfferedInfo) -> void{
      Print::print("Missed publish deadline");
    };

    // Creates the publishers   
    // gamepad_publisher       = this->create_publisher<core::msg::InputGamepad>("/control/input_gamepad", qos, publisher_options);
    gamepad_publisher       = this->create_publisher<core::msg::InputGamepad>("/control/input_gamepad", 1);
    joystick_l_publisher    = this->create_publisher<core::msg::InputJoystick>("/control/input_joystick_l", 1);
    joystick_r_publisher    = this->create_publisher<core::msg::InputJoystick>("/control/input_joystick_r", 1);

    // Creates a timer function that runs a function on loop every 0.02 seconds
    timer = this->create_wall_timer(20ms, std::bind(&InputsPublisher::publish_input, this));

    // Output set-up messages
    Print::title("INPUTS PUBLISHER");
    Print::print("Valid Topics:");
    Print::print("/control/input_gamepad      [InputGamepad]", 1);
    Print::print("/control/input_joystick_l   [InputJoystick]", 1);
    Print::print("/control/input_joystick_r   [InputJoystick]", 1);
    Print::print("", true);
}


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

    // Display information about connections (in case they change)
    if (gamepad->is_disconnected())
        Print::print("Device Disconnected: 'Gamepad'", C_FAIL);
    else if (gamepad->is_reconnected())
        Print::print("Device Connected:    'Gamepad'", C_SUCCESS);
    if (joystick_l->is_disconnected())
        Print::print("Device Disconnected: 'Left Joystick'", C_FAIL);
    else if (joystick_l->is_reconnected())
        Print::print("Device Connected:    'Left Joystick'", C_SUCCESS);
    if (joystick_r->is_disconnected())
        Print::print("Device Disconnected: 'Right Joystick'", C_FAIL);
    else if (joystick_r->is_reconnected())
        Print::print("Device Connected:    'Right Joystick'", C_SUCCESS);
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
