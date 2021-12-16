/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_inputs.h"

// Receives input from left joystick
void ArmInputs::joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg) {
    
    // linear actuator
    linear_actuation = msg->ax_thumb_y;

    // If using the lower joints IK
    if (IK_lower_joints) {
        task_velocity[0] = msg->ax_stick_x;
        task_velocity[1] = msg->ax_stick_y;
        task_velocity[2] = msg->ax_stick_twist;
    }

    // If using standard velocity
    else {
        joint_velocity[0] = msg->ax_stick_twist;
        joint_velocity[1] = msg->ax_stick_x;
        joint_velocity[2] = msg->ax_stick_y;
    }
}


// Receives input from right joystick
void ArmInputs::joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg) {
    
    // end effector actuation
    end_effector_actuation = calculate_direction(msg->ax_thumb_y);

    // Wrist joints
    // If using the wrist IK
    if (IK_wrist) {
        task_velocity[3] = msg->ax_stick_twist;
        task_velocity[4] = msg->ax_stick_x;
        task_velocity[5] = msg->ax_stick_y;
    }

    // If using standard velocity
    else {
        joint_velocity[3] = msg->ax_stick_y;
        joint_velocity[4] = msg->ax_stick_x;
        joint_velocity[5] = msg->ax_stick_twist;
    }

    //Get the speed multiplier from slider
    speed_multiplier = scale_speed(msg->ax_slider); 
}

// Publishes data on the arm input
void ArmInputs::publish_arm_inputs () {
    
    // Create a new message
    auto message = core::msg::ArmInput();

    // Set the values for first 6 joints from the array of data
    for (auto i = 0; i < NUM_JOINTS; i++) {
        message.task_velocity[i]    = speed_multiplier * task_velocity[i];
        message.joint_velocity[i]   = speed_multiplier * joint_velocity[i];
    }

    // Set the values for linear actuator and end effector actuation
    message.linear_actuation = linear_actuation;
    message.end_effector_actuation = end_effector_actuation;

    // Publish the arm inputs
    arm_publisher->publish(message);

    // Reset the raw input data back to 0 to avoid issues
    for (auto i = 0; i < NUM_JOINTS; i++) {
        task_velocity[i]    = 0;
        joint_velocity[i]   = 0;
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

    // Creates the publisher
    arm_publisher = this->create_publisher<core::msg::ArmInput>("/control/arm_input", 10);
    
    // Creates the input subscription for the left joystick
    joystick_l_subscription = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_l", 10, std::bind(&ArmInputs::joystick_l_callback, this, _1));

    // Creates the input subscription for the right joystick
    joystick_r_subscription = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_r", 10, std::bind(&ArmInputs::joystick_r_callback, this, _1));    

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer = this->create_wall_timer(50ms, std::bind(&ArmInputs::publish_arm_inputs, this));
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