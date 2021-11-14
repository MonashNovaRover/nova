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
  - /control/input_gamepad  [InputGamepad] [Subscribed]
  - /control/drive_cmds     [InputGamepad] [Published]
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
#include "core/msg/drive_cmd.hpp"

// Use the standard namespaces
using namespace std;
using namespace std::chrono_literals;

// Main publisher class that sends input data for the gamepad and joysticks
class DrivePublisher : public rclcpp::Node {

    //------------------------------------------------------------//
    private:

        // Stores the loop timer for the update function
        rclcpp::TimerBase::SharedPtr timer;

        // Stores the publishers for each of the controllers
        rclcpp::Publisher<core::msg::DriveCmd>::SharedPtr publisher;

        // Stores a counter
        size_t count;

    
    //------------------------------------------------------------//
    private:

        /// @brief      Publishes the drive commands from analysing
        ///                 the input data.
        void publish_cmds () {
            
            // Publish each of the data streams
            //publisher->publish();
        }


    //------------------------------------------------------------//
    public:

        /// @brief      Default constructor function that starts up the node
        DrivePublisher() : Node("drive_pub"), count(0)
        {
            // Creates the publishers   
            publisher = this->create_publisher<core::msg::DriveCmd>("/control/drive_cmds", 10);
            
            // Creates a timer function that runs a function on loop every 0.01 seconds
            timer = this->create_wall_timer(10ms, std::bind(&DrivePublisher::publish_cmds, this));
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