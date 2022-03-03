/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_control.h"

#include "arm_core.h"
#include "print/print.h"

#include "../hacky_defines.h"

// Receives input from resolvers
void ArmControl::resolver_callback (const sensor_msgs::msg::JointState::SharedPtr msg)
{
}


// Receives joint velocities from arm_inputs node
void ArmControl::joint_vel_callback (const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joint_velocity = *msg;
}

// Receives joint velocities from arm_kinematics node
void ArmControl::joint_vel_ik_callback (const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joint_velocity_ik = *msg;
}

// Publishes data on the desired CMD outputs
void ArmControl::publish_CMD_outputs ()
{    
    // Set the desired cmd velocities by combining joint-space and task-space control info
    for (unsigned int i = 0; i < cmd_outputs.name.size(); i++) {
        cmd_outputs.velocity[i] = joint_velocity.velocity[i] + joint_velocity_ik.velocity[i];
    }

    // Set the header
    cmd_outputs.header.stamp = this->now();
    // Publish the arm inputs
    CMD_outputs_publisher->publish(cmd_outputs);
}

// Main constructor that sets up the node
ArmControl::ArmControl() : Node("arm_control")
{
    // Initialise arrays in internal data structures
    joint_velocity = ArmCore::get_empty_joint_state(hack::JOINT_NAMES);
    joint_velocity_ik = ArmCore::get_empty_joint_state(hack::JOINT_NAMES);
    cmd_outputs = ArmCore::get_empty_joint_state(hack::JOINT_NAMES);

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
        "/electronics/resolvers", 10, std::bind(&ArmControl::resolver_callback, this, _1));

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer = this->create_wall_timer(50ms, std::bind(&ArmControl::publish_CMD_outputs, this));

    // Output set-up messages
    Print::title("ARM CONTROL");
    Print::print("Valid Topics:");
    Print::print("/control/cmd_ouputs            [JointState]", 1);
    Print::print("/control/joint_velocities      [JointState]", 1);
    Print::print("/control/joint_velocities_ik   [JointState]", 1);
    Print::print("/electronics/resolvers         [JointState]", 1);
    Print::print("", true);
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