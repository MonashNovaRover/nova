/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class interfaces with the joystick class to read the input
  data from each of the controllers plugged into the device.
It is able to publish information from the gamepad and both
  joysticks and reads the states of each of the button presses.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: input_pub
TOPICS:
  - /inputs/input_gamepad [InputGamepad]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	  inputs
AUTHOR(S):	Harrison Verrios
CREATION:	  13/11/2021
EDITED:		  24/11/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// General includes
#include "gamepad/gamepad.h"
#include "inputs/joystick_gamepad.h"
#include "inputs/joystick_thrustmaster.h"
#include "inputs/keyboard.h"


// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "input_interfaces/msg/input_gamepad.hpp"
#include "input_interfaces/msg/input_joystick.hpp"
#include "input_interfaces/msg/input_keyboard.hpp"

#include <chrono>
namespace ROSTimers
{
    // Publisher timer periods
    const std::chrono::milliseconds inputs_deadline = 200ms;
    const std::chrono::milliseconds inputs_publish = 20ms;
}


// Main publisher class that sends input data for the gamepad and joysticks
class InputsPublisher : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

    // Stores the loop timer for the update function
    rclcpp::TimerBase::SharedPtr timer;

    // Stores the publishers for each of the controllers
    rclcpp::Publisher<input_interfaces::msg::InputGamepad>::SharedPtr gamepad_publisher;
    rclcpp::Publisher<input_interfaces::msg::InputJoystick>::SharedPtr joystick_l_publisher;
    rclcpp::Publisher<input_interfaces::msg::InputJoystick>::SharedPtr joystick_r_publisher;
    rclcpp::Publisher<input_interfaces::msg::InputKeyboard>::SharedPtr keyboard_publisher;

    // A pointer to the joystick object stored (for the gamepad)
    JoystickGamepad* gamepad;

    // A pointer to the joystick object stored (for the left joystick)
    JoystickThrustmaster* joystick_l;

    // A pointer to the joystick object stored (for the right joystick)
    JoystickThrustmaster* joystick_r;

    // A pointer to the keyboard object stored
    Keyboard* keyboard;

    
    //------------------------------------------------------------//
    private:

    /// @brief      Publishes the input data from the gamepad and
    ///             joystick classes by reading the input.
    void publish_input ();


    //------------------------------------------------------------//
    public:
    
    /// @brief      Default constructor function that starts up the node
    InputsPublisher();
};
