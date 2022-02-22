/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_inputs.h"
#include "print/print.h"

#include "../hacky_defines.h"

// Receives input from left joystick
void ArmInputs::joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg) {
    
    // linear actuator
    linear_actuation = msg->ax_thumb_y;

    //lunar construction
    lunar_construction_left = msg->btn_thumb_u_state;

    // If using the lower joints IK
    if (IK_lower_joints) {
        task_velocity[0] = msg->ax_stick_x;
        task_velocity[1] = msg->ax_stick_y;
        task_velocity[2] = msg->ax_stick_twist;
    }

    // If using standard velocity
    else {
        // Corrected input directions to get correct joint space control scheme
        // May need to revisit once CMD directions determined during IK setup
        joint_velocity[0] = -msg->ax_stick_twist;
        joint_velocity[1] = -msg->ax_stick_y;
        joint_velocity[2] = -msg->ax_stick_x;
    }
}


// Receives input from right joystick
void ArmInputs::joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg) {
    
    // end effector actuation
    end_effector_actuation = calculate_direction(msg->ax_thumb_y) * 0.95;

    //lunar construction
    lunar_construction_right = msg->btn_thumb_u_state;

    // Wrist joints
    // If using the wrist IK
    if (IK_wrist) {
        task_velocity[3] = -msg->ax_stick_y;
        task_velocity[4] = msg->ax_stick_x;
        task_velocity[5] = msg->ax_stick_twist;
    }

    // If using standard velocity
    else {
        // Corrected input directions to get correct joint space control scheme
        // May need to revisit once CMD directions determined during IK setup
        joint_velocity[3] = -msg->ax_stick_x;
        joint_velocity[4] = msg->ax_stick_y;
        joint_velocity[5] = -msg->ax_stick_twist;
    }

    //Get the speed multiplier from slider
    speed_multiplier = scale_speed(msg->ax_slider); 
}

// Publishes data on the arm input
void ArmInputs::publish_arm_inputs () {
    
    // Create a new message
    auto message = core::msg::ArmInput();

    /*
    // Set the values for first 6 joints from the array of data
    for (auto i = 0; i < NUM_JOINTS; i++) {
        message.task_velocity[i]    = speed_multiplier * task_velocity[i];
        message.joint_velocity[i]   = speed_multiplier * joint_velocity[i];
    }
    */

    // Set the values for linear actuator and end effector actuation
    message.linear_actuation = linear_actuation;
    message.end_effector_actuation = end_effector_actuation;

    // Set the values for lunar construction
    if (lunar_construction_left == 2) {
        message.lunar_construction = 1;
    }
    else if (lunar_construction_right == 2) {
        message.lunar_construction = -1;
    }
    else {
        message.lunar_construction = 0;
    }
    

    // Publish the arm inputs
    arm_publisher->publish(message);

    // Reset the raw input data back to 0 to avoid issues
    /*
    for (auto i = 0; i < NUM_JOINTS; i++) {
        task_velocity[i]    = 0;
        joint_velocity[i]   = 0;
    }
    */
}

// Publishes joint velocity data
void ArmInputs::publish_joint_vel () {

    // create a new message
    // auto message = sensor_msgs::msg::JointState();

    // Set the header
    joint_velocities.header.stamp = this->now();

    // Set the values for first 6 joints from the array of joint space data
    for (auto i = 0; i < NUM_JOINTS; i++) {
        joint_velocities.velocity[i]   = speed_multiplier * joint_velocity[i];
        
        // std::cout << "raw" << joint_velocity[1] << std::endl;
        // std::cout << "not raw" <<joint_velocities.velocity[1] << std::endl;
    }

    // Publish the joint space velocities
    joint_vel_publisher->publish(joint_velocities);

    // Reset the raw input data back to 0 to avoid issues
    for (auto i = 0; i < NUM_JOINTS; i++) {
        joint_velocity[i]   = 0;
    }
}

// Publishes task velocity data
void ArmInputs::publish_task_vel () {

    // create a new message
    // auto message = geometry_msgs::msg::TwistStamped();

    // Set the header
    task_velocities.header.stamp = this->now();

    // Set the values for linear velocity
    task_velocities.twist.linear.x = speed_multiplier * task_velocity[0];
    task_velocities.twist.linear.y = speed_multiplier * task_velocity[1];
    task_velocities.twist.linear.z = speed_multiplier * task_velocity[2];
    task_velocities.twist.angular.x = speed_multiplier * task_velocity[3];
    task_velocities.twist.angular.y = speed_multiplier * task_velocity[4];
    task_velocities.twist.angular.z = speed_multiplier * task_velocity[5];

    // Publish the joint space velocities
    task_vel_publisher->publish(task_velocities);

    // Reset the raw input data back to 0 to avoid issues
    for (auto i = 0; i < NUM_JOINTS; i++) {
        task_velocity[i]   = 0;
    }
}

float ArmInputs::calculate_direction (float value){
    if (value > 0){
        return 1.0;
    }
    else if (value < 0){
        return -1.0;
    }
    else{
        return 0.0;
    }
}


float ArmInputs::scale_speed (float value){
    //max scale factor 0.95, min scale factor 0.05
    // return (((value - 1) / -2.0) * 0.9) + 0.05;
    return (value * 0.9) + 0.05;
}


// Main constructor that sets up the node
ArmInputs::ArmInputs() 
  : Node("arm_pub"), count(0) {

    // Initialise arrays in internal data structures
    joint_velocities = ArmCore::get_empty_joint_state(hack::JOINT_NAMES);

    // Creates the arm inputs publisher
    arm_publisher = this->create_publisher<core::msg::ArmInput>("/control/arm_input", 10);

    // Creates the joint velocity publisher
    joint_vel_publisher = this->create_publisher<sensor_msgs::msg::JointState>("/control/joint_velocities", 10);

    // Creates the task velocity publisher
    task_vel_publisher = this->create_publisher<geometry_msgs::msg::TwistStamped>("/control/task_velocity", 10);
    
    // Creates the input subscription for the left joystick
    joystick_l_subscription = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_l", 10, std::bind(&ArmInputs::joystick_l_callback, this, _1));

    // Creates the input subscription for the right joystick
    joystick_r_subscription = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_r", 10, std::bind(&ArmInputs::joystick_r_callback, this, _1));    

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer = this->create_wall_timer(50ms, std::bind(&ArmInputs::publish_arm_inputs, this));

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer_joint = this->create_wall_timer(50ms, std::bind(&ArmInputs::publish_joint_vel, this));

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer_task = this->create_wall_timer(50ms, std::bind(&ArmInputs::publish_task_vel, this));

    // Output set-up messages
    Print::title("ARM INPUTS");
    Print::print("Valid Topics:");
    Print::print("/control/arm_input            [ArmInput]", 1);
    Print::print("/control/joint_velocities     [JointState]", 1);
    Print::print("/control/task_velocity        [TwistStamped]", 1);
    Print::print("", true);
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmInputs>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}