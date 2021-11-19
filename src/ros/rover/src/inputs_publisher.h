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
  - /control/input_gamepad [InputGamepad]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	  control
AUTHOR(S):	Harrison Verrios
CREATION:	  13/11/2021
EDITED:		  19/11/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Implement joystick controls (along with gamepad)
 - Create automatic assigning of joysticks with correct device
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// General includes
#include <gamepad/gamepad.h>
#include "joystick.h"

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_gamepad.hpp"
#include "core/msg/input_joystick.hpp"

// Use the standard namespaces
using namespace std;
using namespace std::chrono_literals;


// Main publisher class that sends input data for the gamepad and joysticks
class InputsPublisher : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

    // Stores the loop timer for the update function
    rclcpp::TimerBase::SharedPtr timer;

    // Stores the publishers for each of the controllers
    rclcpp::Publisher<core::msg::InputGamepad>::SharedPtr gamepad_publisher;
    rclcpp::Publisher<core::msg::InputJoystick>::SharedPtr joystick_l_publisher;
    rclcpp::Publisher<core::msg::InputJoystick>::SharedPtr joystick_r_publisher;

    // Stores a counter
    size_t count;

    // A pointer to the joystick object stored (for the gamepad)
    JoystickGamepad* gamepad;

    // A pointer to the joystick object stored (for the left joystick)
    JoystickThrustmaster* joystick_l;

    // A pointer to the joystick object stored (for the right joystick)
    JoystickThrustmaster* joystick_r;

    
    //------------------------------------------------------------//
    private:

    /// @brief      Publishes the input data from the gamepad and
    ///                 joystick classes by reading the input.
    void publish_input ();


    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    InputsPublisher();

};