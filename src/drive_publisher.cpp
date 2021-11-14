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

// The minimum and maximum multipliers
const float MIN_MULTIPLIER      = 0.1;  // The minimum multiplier value
const float MAX_MULTIPLIER      = 1.0;  // The maximum multiplier value
const float DELTA_MULTIPLIER    = 0.1;  // The change in multiplier

// The minimum trigger speed multiplier to apply when the right trigger is held
const float MIN_TRIGGER_MULTIPLIER = 0.4;

/*
    HOW TO DRIVE THE ROVER:

    Left Y Axis:    Speed (forwards and backwards)
    Right X Axis:   Steer (left and right)
    DPAD Y Axis:    Increase / Decrease the Speed multiplier by 10%
    DPAD X Axis:    Increase / Decrease the Steer multiplier by 10%
    Right Trigger:  Add Custom speed multipliers between 1.0 and 0.4
*/


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

        // Stores the current state of the trigger multiplier
        float trigger_speed = 1.0;

        // The current speed and steer multipliers
        float multiplier_speed = 0.5;
        float multiplier_steer = 1.0;

    
    //------------------------------------------------------------//
    private:

        /// @brief      Adjusts one of the multipliers between 0.1 and 1.0
        /// @param      multiplier - A reference to the speed or steer multiplier
        /// @param      increase - A boolean flag for increasing (or false to decrease)
        /// @returns    The current value of the multiplier
        float adjust_multiplier (float& multiplier, bool increase) {
            multiplier += (increase) ? DELTA_MULTIPLIER : -DELTA_MULTIPLIER;

            // Check for minimum and maximums
            if (multiplier > MAX_MULTIPLIER)
                multiplier = MAX_MULTIPLIER;
            else if (multiplier <= MIN_MULTIPLIER)
                multiplier = MIN_MULTIPLIER;

            // Return the new multiplier
            return multiplier;
        }


        /// @brief      Publishes the drive commands from analysing
        ///                 the input data.
        void publish_cmds () {

            // Create the message
            auto message = core::msg::DriveCmd();

            // Set up the values
            message.speed = input_axis_y * multiplier_speed * trigger_speed;
            message.steer = input_axis_x * multiplier_steer;
            
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
                trigger_speed = 1.0;
            }

            // If the controller is connected
            else {
                // Update the input axis
                input_axis_x = msg->ax_stick_r_x;
                input_axis_y = msg->ax_stick_l_y;

                // Get the trigger speed multiplier
                trigger_speed = 1.0 - (msg->trg_r_val * (1 - MIN_TRIGGER_MULTIPLIER));

                // Change the speed multipliers
                if (msg->btn_dpad_u_state == 1)
                    adjust_multiplier(multiplier_speed, true);
                else if (msg->btn_dpad_d_state == 1)
                    adjust_multiplier(multiplier_speed, false);
                
                // Change the steer multipliers
                if (msg->btn_dpad_r_state == 1)
                    adjust_multiplier(multiplier_steer, true);
                else if (msg->btn_dpad_l_state == 1)
                    adjust_multiplier(multiplier_steer, false);
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