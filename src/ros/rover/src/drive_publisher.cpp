/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class reads data from the inputs from the game
    controllers and is able to publish drive commands
    based on what the controller data tells us.
The drive commands will be a RPM (desired) and a steer
    factor. Each of these will be a value between 0
    and 1.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: drive_pub
TOPICS:
  - /control/input_gamepad  [InputGamepad]  [Subscribed]
  - /control/drive_cmds     [DriveCmd]      [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Harrison Verrios
CREATION:	14/11/2021
EDITED:		14/11/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Test with the driver received code (not yet created).
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_gamepad.hpp"
#include "core/msg/drive_cmd.hpp"

#include <iostream>

// Use the standard namespaces
using namespace std;
using namespace std::chrono_literals;
using std::placeholders::_1;

// Main publisher class that sends input data for the gamepad and joysticks
class DrivePublisher : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

        // Stores the loop timer for the update function
        rclcpp::TimerBase::SharedPtr timer;

        // Stores the publisher for the drive commands
        rclcpp::Publisher<core::msg::DriveCmd>::SharedPtr publisher;

        // Stores the subscriber to the gamepad inputs
        rclcpp::Subscription<core::msg::InputGamepad>::SharedPtr subscription;

        // Stores a counter for each step
        size_t count;

        // Stores the current state of the input axis
        float input_axis_x = 0.0;
        float input_axis_y = 0.0;

    
    //------------------------------------------------------------//
    private:

        /// @brief      Publishes the drive commands from analysing
        ///                 the input data.
        void publish_cmds () {

            // Create the message
            auto message = core::msg::DriveCmd();

            // Set up the values
            message.speed = input_axis_y;
            message.steer = input_axis_x;
            
            // Publish the drive commands
            publisher->publish(message);
        }

        /// @brief      Callback function when input messages are received.
        /// @param      msg - A pointer to the input message
        void input_callback (const core::msg::InputGamepad::SharedPtr msg) {
            // If no connection, reset the state
            if (!msg->connected) {
                input_axis_x = 0.0;
                input_axis_y = 0.0;
            }

            // Update the input axis
            else {
                input_axis_x = msg->ax_stick_r_x;
                input_axis_y = msg->ax_stick_l_y;
            }         
        }


    //------------------------------------------------------------//
    public:

        /// @brief      Default constructor function that starts up the node
        DrivePublisher() : Node("drive_pub"), count(0)
        {
            // Creates the publisher
            publisher = this->create_publisher<core::msg::DriveCmd>("/control/drive_cmds", 10);
            
            // Creates the input subscription
            subscription = this->create_subscription<core::msg::InputGamepad>(
                "/control/input_gamepad", 10, std::bind(&DrivePublisher::input_callback, this, _1));

            // Creates a timer function that runs a function on loop every 0.05 seconds
            timer = this->create_wall_timer(50ms, std::bind(&DrivePublisher::publish_cmds, this));
        }
};



//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<DrivePublisher>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}