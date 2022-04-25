/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jess Hepworth
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "arm_inputs.h"

#include "arm_core.h"
#include "print/print.h"

#include "../hacky_defines.h"

// Receives input from left joystick
void ArmInputs::joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg)
{
    // Save data for later, only deal with it when we publish
    // More efficient, works if we only care about the most up-to-date message
    joystick_l = *msg;

    // Set button-based data here so we don't miss any button-press events
    
    // Arm lock
    if (joystick_l.btn_thumb_l_state == 1) {
        if (!control_scheme.joystick_lock)
            Print::print("Joysticks locked");
        control_scheme.joystick_lock = true;
    }
    if (joystick_l.btn_thumb_r_state == 1){
        if (control_scheme.joystick_lock)
            Print::print("Joysticks Unlocked");
        control_scheme.joystick_lock = false;
    }
    
    // Control schemes
    bool control_scheme_update = false;
    // IK
    if (joystick_l.btn_bottom_r1_state == 1) {
        control_scheme.ik_linear = !control_scheme.ik_linear;
        control_scheme_update = true;
    }
    if (joystick_l.btn_bottom_r4_state == 1) {
        control_scheme.ik_angular = !control_scheme.ik_angular;
        control_scheme_update = true;
    }
    // Endpoint frame control
    if (joystick_l.btn_bottom_r2_state == 1) {
        control_scheme.endpoint_frame_linear = !control_scheme.endpoint_frame_linear;
        control_scheme_update = true;
    }
    if (joystick_l.btn_bottom_r5_state == 1) {
        control_scheme.endpoint_frame_angular = !control_scheme.endpoint_frame_angular;
        control_scheme_update = true;
    }
    // Joint limits
    if (joystick_l.btn_bottom_l1_state == 1) {
        control_scheme.joint_limits = true;
        control_scheme_update = true;
    }
    if (joystick_l.btn_bottom_l4_state == 1) {
        control_scheme.joint_limits = false;
        control_scheme_update = true;
    }
    // Immediately publish any new control scheme data
    // Also will continue to publish when the timer expires
    if (control_scheme_update){
        publish_control_scheme();
    }

}

// Receives input from right joystick
void ArmInputs::joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg)
{
    // Save data for later, only deal with it when we publish
    // More efficient, works if we only care about the most up-to-date message    
    joystick_r = *msg;
}

// Resets joystick internal state
void ArmInputs::joystick_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "Joystick subscriber deadline missed");
    joystick_l = core::msg::InputJoystick();
    joystick_r = core::msg::InputJoystick();
}

// Publishes data on the arm input
void ArmInputs::publish_endeffector_inputs ()
{
    // Create a new message
    auto message = core::msg::EndEffectorInput();

    if (!control_scheme.joystick_lock){
        // First 6 joints are handled separately
        
        // Set the values for linear actuator and end effector actuation
        message.linear_actuation = joystick_l.ax_thumb_x;
        message.end_effector_actuation = calculate_direction(joystick_r.ax_thumb_x) * 0.95;
    }
    
    // Publish the arm inputs
    endeffector_publisher->publish(message);
}

// Publishes joint velocity data
void ArmInputs::publish_joint_vel ()
{
    // Get the speed from slider, apply scaling
    float speed = scale_speed(joystick_r.ax_slider) * speed_multipliers.all_inputs;
    
    // If using lower joints joint-space control
    if (!control_scheme.joystick_lock && !control_scheme.ik_linear) {
        // No speed scaling for lower joints;
        
        // Base rotation is stick twist. CCW rotates arm CCW (from above)
        joint_velocities.velocity[0] = speed * joystick_l.ax_stick_twist;
        // Shoulder is stick y (left-right). Left moves the arm towards the back of the rover
        joint_velocities.velocity[1] = speed * joystick_l.ax_stick_y;
        // Elbow is stick x (forward-backward). Forward pitches arm down
        joint_velocities.velocity[2] = speed * -joystick_l.ax_stick_x;
    }
    else{
        joint_velocities.velocity[0] = 0;
        joint_velocities.velocity[1] = 0;
        joint_velocities.velocity[2] = 0;
    }

    // If using wrist joint-space control
    if (!control_scheme.joystick_lock && !control_scheme.ik_angular) {
        // Scale speed for wrist joints
        float speed_wrist_joints = speed * speed_multipliers.wrist_joints;
        
        // J4 is stick x. Forward pitches arm down
        joint_velocities.velocity[3] = speed_wrist_joints * -joystick_r.ax_stick_x;
        // J5 is stick y. Left yaws arm left
        joint_velocities.velocity[4] = speed_wrist_joints * joystick_r.ax_stick_y;
        // J6 is stick twist. CCW tilts end effector CCW (looking out from end effector)
        joint_velocities.velocity[5] = speed_wrist_joints * -joystick_r.ax_stick_twist;
    }
    else{
        joint_velocities.velocity[3] = 0;
        joint_velocities.velocity[4] = 0;
        joint_velocities.velocity[5] = 0;
    }

    // Set the header
    joint_velocities.header.stamp = this->now();
    // Publish the joint space velocities
    joint_vel_publisher->publish(joint_velocities);
}

// Publishes task velocity data
void ArmInputs::publish_task_vel ()
{
    // Get the speed from slider, apply scaling
    float speed = scale_speed(joystick_r.ax_slider) * speed_multipliers.all_inputs;
    
    // If using lower joints IK, set the values for linear velocity
    if (!control_scheme.joystick_lock && control_scheme.ik_linear) {
        // Scale speed for linear IK
        float speed_ik_linear = speed * speed_multipliers.ik_linear;

        // Linear velocities map directly from joystick. Directions are already in arm base coords
        task_velocities.twist.linear.x = speed_ik_linear * joystick_l.ax_stick_x;
        task_velocities.twist.linear.y = speed_ik_linear * joystick_l.ax_stick_y;
        task_velocities.twist.linear.z = speed_ik_linear * joystick_l.ax_stick_twist;
    }
    else {
        task_velocities.twist.linear.x = 0;
        task_velocities.twist.linear.y = 0;
        task_velocities.twist.linear.z = 0;
    }
    // If using wrist IK, set the values for angular velocity
    if (!control_scheme.joystick_lock && control_scheme.ik_angular) {
        // Scale speed for angular IK
        float speed_ik_angular = speed * speed_multipliers.ik_angular;
        
        // Adjust roll and pitch directions so control is more intuitive
        // Equivalent to a rotation of the input angular velocity vector by +pi/2 about z axis
        // Roll is stick y (left-right)
        task_velocities.twist.angular.x = speed_ik_angular * -joystick_r.ax_stick_y;
        // Pitch is stick x (forward-backward)
        task_velocities.twist.angular.y = speed_ik_angular * joystick_r.ax_stick_x;
        // Yaw is stick twist
        task_velocities.twist.angular.z = speed_ik_angular * joystick_r.ax_stick_twist;
    }
    else{
        task_velocities.twist.angular.x = 0;
        task_velocities.twist.angular.y = 0;
        task_velocities.twist.angular.z = 0;
    }

    // Set the header
    task_velocities.header.stamp = this->now();
    // Publish the joint space velocities
    task_vel_publisher->publish(task_velocities);
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
    // Max scale factor 1.00, min scale factor 0.05
    return (value * 0.95) + 0.05;
}

// Publishes control scheme data
void ArmInputs::publish_control_scheme()
{   
    // Buttons are handled separately
    
    // Set base reference frame offset
    int8_t base_frame_offset = 0;
    if (joystick_l.ax_slider < 0.3) {
        base_frame_offset = -1;
    }
    else if (joystick_l.ax_slider > 0.8) {
        base_frame_offset = 1;
    }
    control_scheme.base_frame_offset = base_frame_offset;
    
    // Set the header and publish
    control_scheme.header.stamp = this->now();
    control_scheme_publisher->publish(control_scheme);
}


// Main constructor that sets up the node
ArmInputs::ArmInputs() : Node("arm_inputs")
{
    // Creates the end effector inputs publisher
    endeffector_publisher = this->create_publisher<core::msg::EndEffectorInput>(
        "/control/endeffector_input", rclcpp::QoS(1).best_effort().deadline(200ms)
    );

    // Creates the joint velocity publisher
    joint_vel_publisher = this->create_publisher<sensor_msgs::msg::JointState>(
        "/control/input_joint_velocities", rclcpp::QoS(1).best_effort().deadline(200ms)
    );

    // Creates the task velocity publisher
    task_vel_publisher = this->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/control/task_velocity", rclcpp::QoS(1).best_effort().deadline(200ms)
    );

    // Create common options for joystick subscriptions
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> joystick_options;
    joystick_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->joystick_deadline_callback();
    };
    rclcpp::QoS joystick_qos = rclcpp::QoS(1).best_effort().deadline(200ms);

    // Creates the input subscription for the left joystick (with QoS options)
    joystick_l_subscription = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_l",
        joystick_qos,
        std::bind(&ArmInputs::joystick_l_callback, this, _1),
        joystick_options
    );

    // Creates the input subscription for the right joystick (with QoS options)
    joystick_r_subscription = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_r",
        joystick_qos,
        std::bind(&ArmInputs::joystick_r_callback, this, _1),
        joystick_options
    );

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer = this->create_wall_timer(50ms, std::bind(&ArmInputs::publish_endeffector_inputs, this));

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer_joint = this->create_wall_timer(50ms, std::bind(&ArmInputs::publish_joint_vel, this));

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer_task = this->create_wall_timer(50ms, std::bind(&ArmInputs::publish_task_vel, this));

    // Create timer and publisher for control_scheme
    control_scheme_timer = this->create_wall_timer(
        50ms, std::bind(&ArmInputs::publish_control_scheme, this)
    );
    control_scheme_publisher = this->create_publisher<core::msg::ArmControlScheme>(
        "/control/arm_control_scheme", 10
    );

     // Initialise arrays in internal data structures
    joint_velocities = ArmCore::get_empty_joint_state(hack::JOINT_NAMES);
    
    // Publish the control scheme to initialise other nodes
    // Uses the default field values
    publish_control_scheme();

    // Output set-up messages
    Print::title("ARM INPUTS");
    Print::print("Subscribed Topics:");
    Print::print("/control/input_joystick_l         [core/InputJoystick]", 1);
    Print::print("/control/input_joystick_r         [core/InputJoystick]", 1);
    Print::print("Published Topics:");
    Print::print("/control/endeffector_input        [core/EndEffectorInput]", 1);
    Print::print("/control/input_joint_velocities   [sensor_msgs/JointState]", 1);
    Print::print("/control/task_velocity            [sensor_msgs/TwistStamped]", 1);
    Print::print("/control/arm_control_scheme       [core/ArmControlScheme]", 1);
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
