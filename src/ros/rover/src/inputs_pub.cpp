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
EDITED:		  14/11/2021
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

// Use the standard namespaces
using namespace std;
using namespace std::chrono_literals;

// Main publisher class that sends input data for the gamepad and joysticks
class InputPublisher : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

        // Stores the loop timer for the update function
        rclcpp::TimerBase::SharedPtr timer;

        // Stores the publishers for each of the controllers
        rclcpp::Publisher<core::msg::InputGamepad>::SharedPtr gamepad_publisher;

        // Stores a counter
        size_t count;

        // A pointer to the joystick object stored (for the gamepad)
        Joystick* gamepad;

    
    //------------------------------------------------------------//
    private:

        /// @brief      Publishes the input data from the gamepad and
        ///                 joystick classes by reading the input.
        void publish_input () {
            // Updates the state of the gamepad
            GamepadUpdate();

            // Update the status of each controller
            gamepad->Update();
            
            // Publish each of the data streams
            gamepad_publisher->publish(gamepad->GetMessage());
        }


    //------------------------------------------------------------//
    public:

        /// @brief      Default constructor function that starts up the node
        InputPublisher() : Node("input_pub"), count(0)
        {
            // Initialises the Gamepad inputs
            GamepadInit();

            // Creates all the joysticks
            gamepad = new Joystick(GAMEPAD_0);  

            // Creates the publishers   
            gamepad_publisher = this->create_publisher<core::msg::InputGamepad>("/control/input_gamepad", 10);
            
            // Creates a timer function that runs a function on loop every 0.01 seconds
            timer = this->create_wall_timer(10ms, std::bind(&InputPublisher::publish_input, this));
        }
};



//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<InputPublisher>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}