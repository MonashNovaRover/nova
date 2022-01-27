/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_control.h"
#include "print/print.h"

// Receives input from resolvers
void ArmControl::resolver_callback (const sensor_msgs::msg::JointState::SharedPtr msg) {
    
}


// Receives joint velocities from arm_inputs node
void ArmControl::joint_vel_callback (const sensor_msgs::msg::JointState::SharedPtr msg) {
    
    for (auto i = 0; i < NUM_JOINTS; i++) {
        joint_velocity[i] = msg->velocity[i];
    }

}

// Receives joint velocities from arm_kinematics node
void ArmControl::joint_vel_ik_callback (const sensor_msgs::msg::JointState::SharedPtr msg) {
    
    for (auto i = 0; i < NUM_JOINTS; i++) {
        joint_velocity_ik[i] = msg->velocity[i];
    }

}

// Publishes data on the desired CMD outputs
void ArmControl::publish_CMD_outputs () {
    
    // Create a new message
    auto message = sensor_msgs::msg::JointState();

    for (auto i = 0; i < NUM_JOINTS; i++) {
        message.velocity[i]   = joint_velocity[i] + joint_velocity_ik[i];
    }

    // Publish the arm inputs
    CMD_outputs_publisher->publish(message);

}

// Main constructor that sets up the node
ArmControl::ArmControl() 
  : Node("arm_control"), count(0) {

    // Creates the CMD outputs publisher
    CMD_outputs_publisher = this->create_publisher<sensor_msgs::msg::JointState>("/control/cmd_outputs", 10);

    // Creates the subscription for joint velocities from arm_kinematics node
    joint_vel_ik_subscription = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/joint_velocities_ik", 10, std::bind(&ArmControl::joint_vel_ik_callback, this, _1));

    // Creates the subscription for joint velocities from arm_inputs node
    joint_vel_subscription = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/joint_velocities", 10, std::bind(&ArmControl::joint_vel_callback, this, _1));   

     // Creates the subscription for resolvers
    resolvers_subscription = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/resolvers", 10, std::bind(&ArmControl::resolver_callback, this, _1));

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer = this->create_wall_timer(50ms, std::bind(&ArmControl::publish_CMD_outputs, this));

    for (auto i = 0; i < NUM_JOINTS; i++) {
        joint_velocity[i] = 0; 
        joint_velocity_ik[i] = 0; 
    }

}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmControl>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}