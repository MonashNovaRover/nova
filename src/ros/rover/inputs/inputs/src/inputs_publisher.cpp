/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	inputs
AUTHOR(S):	Harrison Verrios
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "inputs/inputs_publisher.h"
#include "print/print.h"


// Main consrtuctor sets up the node and the publishers
InputsPublisher::InputsPublisher() : Node("input_pub")
{
    // Initialises the Gamepad inputs
    // Pass in the Joystick Identifier. You can find this by:
    //  cat /sys/class/input/js2/device/id/version on the correct js2 (input device)
    //  and then use VENDOR:PRODUCT
    GamepadInit("044F:B10A");

    // Creates all the joysticks (gamepads and thrustmasters) 
    gamepad     = new JoystickGamepad(0.0);
    joystick_l  = new JoystickThrustmaster(true, 0.06445, 0.5);
    joystick_r  = new JoystickThrustmaster(false, 0.06445, 0.0);
    keyboard    = new Keyboard();

    // Creates the publishers
    // gamepad_publisher       = this->create_publisher<input_interfaces::msg::InputGamepad>("/inputs/input_gamepad", qos, publisher_options);
    gamepad_publisher       = this->create_publisher<input_interfaces::msg::InputGamepad>("/inputs/input_gamepad", rclcpp::QoS(1).best_effort().deadline(ROSTimers::inputs_deadline));
    joystick_l_publisher    = this->create_publisher<input_interfaces::msg::InputJoystick>("/inputs/input_joystick_l", rclcpp::QoS(1).best_effort().deadline(ROSTimers::inputs_deadline));
    joystick_r_publisher    = this->create_publisher<input_interfaces::msg::InputJoystick>("/inputs/input_joystick_r", rclcpp::QoS(1).best_effort().deadline(ROSTimers::inputs_deadline));
    keyboard_publisher      = this->create_publisher<input_interfaces::msg::InputKeyboard>("/inputs/input_keyboard", rclcpp::QoS(1).best_effort().deadline(ROSTimers::inputs_deadline));

    // Creates a timer function that runs a function on loop
    timer = this->create_wall_timer(ROSTimers::inputs_publish, std::bind(&InputsPublisher::publish_input, this));

    // Output set-up messages
    Print::title("INPUTS PUBLISHER");
    Print::print("Valid Topics:");
    Print::print("/inputs/input_gamepad      [InputGamepad]", 1);
    Print::print("/inputs/input_joystick_l   [InputJoystick]", 1);
    Print::print("/inputs/input_joystick_r   [InputJoystick]", 1);
    Print::print("/inputs/input_keyboard     [InputKeyboard]", 1);
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
    keyboard ->update();

    // Display information about connections (in case they change)
    if (gamepad->is_disconnected())
        Print::print("Device Disconnected: 'Gamepad'", C_FAIL);
    else if (gamepad->is_reconnected())
        Print::print("Device Connected:    'Gamepad'", C_SUCCESS);

    if (joystick_l->is_disconnected()) {
        Print::print("Device Disconnected: 'Left Joystick'", C_FAIL);
        joystick_l->reset_inputs();
    }
    else if (joystick_l->is_reconnected())
        Print::print("Device Connected:    'Left Joystick'", C_SUCCESS);

    if (joystick_r->is_disconnected()) {
        Print::print("Device Disconnected: 'Right Joystick'", C_FAIL);
        joystick_r->reset_inputs();
    }
    else if (joystick_r->is_reconnected())
        Print::print("Device Connected:    'Right Joystick'", C_SUCCESS);

    if (keyboard->is_disconnected())
        Print::print("Device Disconnected: 'Keyboard'", C_FAIL);
    else if (keyboard->is_reconnected())
        Print::print("Device Connected:    'Keyboard'", C_SUCCESS);
        
    // Publish each of the data streams
    gamepad_publisher->publish(gamepad->get_message());
    joystick_l_publisher->publish(joystick_l->get_message());
    joystick_r_publisher->publish(joystick_r->get_message());
    keyboard_publisher->publish(keyboard->get_message());
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
