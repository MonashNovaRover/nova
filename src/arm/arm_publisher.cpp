/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_publisher.h"

// Receives input from left joystick
void ArmPublisher::joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg) {
    
    // If using the wrist IK
    if (IK_wrist) {
        task_velocity[0] = msg->ax_stick_twist;
        task_velocity[1] = msg->ax_stick_x;
        task_velocity[2] = msg->ax_stick_y;
    }

    // If using standard velocity
    else {
        joint_velocity[0] = msg->ax_stick_twist;
        joint_velocity[1] = msg->ax_stick_x;
        joint_velocity[2] = msg->ax_stick_y;
    }
}


// Receives input from left joystick
void ArmPublisher::joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg) {
    
    // If using the wrist IK
    if (IK_wrist) {
        task_velocity[3] = msg->ax_stick_twist;
        task_velocity[4] = msg->ax_stick_x;
        task_velocity[5] = msg->ax_stick_y;
    }

    // If using standard velocity
    else {
        joint_velocity[3] = msg->ax_stick_twist;
        joint_velocity[4] = msg->ax_stick_x;
        joint_velocity[5] = msg->ax_stick_y;
    }
}

// Publishes data on the arm input
void ArmPublisher::publish_arm_inputs () {
    
    // Create a new message
    auto message = core::msg::ArmInput();

    // Set the values from the array of data
    for (auto i = 0; i < NUM_JOINTS; i++) {
        message.task_velocity[i]    = task_velocity[i];
        message.joint_velocity[i]   = joint_velocity[i];
    }

    // Publish the arm inputs
    arm_publisher->publish(message);

    // Reset the raw input data back to 0 to avoid issues
    for (auto i = 0; i < NUM_JOINTS; i++) {
        task_velocity[i]    = 0;
        joint_velocity[i]   = 0;
    }
}


// Main constructor that sets up the node
ArmPublisher::ArmPublisher() 
  : Node("arm_pub"), count(0) {

    // Creates the publisher
    arm_publisher = this->create_publisher<core::msg::ArmInput>("/control/arm_input", 10);
    
    // Creates the input subscription for the left joystick
    joystick_l_subscription = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_l", 10, std::bind(&ArmPublisher::joystick_l_callback, this, _1));

    // Creates the input subscription for the right joystick
    joystick_r_subscription = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_r", 10, std::bind(&ArmPublisher::joystick_r_callback, this, _1));    

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer = this->create_wall_timer(50ms, std::bind(&ArmPublisher::publish_arm_inputs, this));
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmPublisher>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}