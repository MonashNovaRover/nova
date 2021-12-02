/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	
AUTHOR(S):	
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_input.h"

// Include standard output messages
#include <iostream>

// Receives input from left joystick
void ArmPublisher::joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg) {
      if (IK_lower_joints) {
          task_velocity[0] = msg->ax_stick_twist
          task_velocity[1] = msg->ax_stick_x
          task_velocity[2] = msg->ax_stick_y
      }
      else{
          joint_velocity[0] = msg->ax_stick_twist
          joint_velocity[1] = msg->ax_stick_x
          joint_velocity[2] = msg->ax_stick_y
      }
}

// Receives input from left joystick
void ArmPublisher::joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg) {
       if (IK_wrist) {
          task_velocity[3] = msg->ax_stick_twist
          task_velocity[4] = msg->ax_stick_x
          task_velocity[5] = msg->ax_stick_y
      }
      else{
          joint_velocity[3] = msg->ax_stick_twist
          joint_velocity[4] = msg->ax_stick_x
          joint_velocity[5] = msg->ax_stick_y
      }

}

void ArmPublisher::publish_arm_inputs () {
    
    auto message = msg::core::ArmInput();

    message.task_velocity = task_velocity;
    message.joint_velocity = joint_velocity;

    // Publish the arm inputs
    publisher->publish(message);

    // Reset velocities 
    task_velocity = {0, 0, 0, 0, 0, 0}
    joint_velocity = {0, 0, 0, 0, 0, 0}

}

// Main constructor that sets up the node
ArmPublisher::ArmPublisher() 
  : Node("arm_pub"), count(0) {

    // Creates the publisher
    arm_input_publisher = this->create_publisher<core::msg::ArmInput>("/control/arm_input", 10);
    
    // Creates the input subscription
    joystick_l_subscription = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_l", 10, std::bind(&ArmPublisher::joystick_l_callback, this, _1));

    // Creates the input subscription
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