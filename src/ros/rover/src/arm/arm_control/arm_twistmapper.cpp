/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_twistmapper.h"

#include "arm_messages.h"
#include "arm_type_translation.h"
#include "../arm_configuration.h"
#include "print/print.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <string>

ArmTwistMapper::ArmTwistMapper() :
    Node("arm_twist_mapper"),
    // Transform joystick input directions to intuitive end-effector coordinates
    // eg: forward on the left joystick is +ve x, but should be +ve z in end effector coordinates
    ENDPOINT_INPUT_TRANSFORM_LINEAR(KDL::Rotation::EulerZYX(M_PI / 2, -M_PI / 2, 0)),
    // Switch yaw and roll directions for more intuitive control
    ENDPOINT_INPUT_TRANSFORM_ANGULAR(KDL::Rotation::RotX(M_PI / 2) * ENDPOINT_INPUT_TRANSFORM_LINEAR)
{    
    // Initialise publish timer periods
    control_pub_timer_period = 50ms;
    
    
    // Create subscription to arm control scheme
    control_scheme_sub = this->create_subscription<core::msg::ArmControlScheme>(
        "/control/arm_control_scheme", 10, std::bind(&ArmTwistMapper::control_scheme_callback, this, _1)
    );

    // Create subscription to resolvers
    resolver_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/electronics/resolvers", 10, std::bind(&ArmTwistMapper::resolver_callback, this, _1)
    );
    
    // Create subscription to joystick_joint_velocities
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> joystick_joint_velocities_options;
    joystick_joint_velocities_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->joystick_joint_velocities_deadline_callback();
    };
    joystick_joint_velocities_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/joystick_joint_velocities",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmTwistMapper::joystick_joint_velocities_callback, this, _1),
        joystick_joint_velocities_options
    );

    // Create subscription to joystick_twist
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> joystick_twist_options;
    joystick_twist_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->joystick_twist_deadline_callback();
    };
    joystick_twist_sub = this->create_subscription<geometry_msgs::msg::TwistStamped>(
        "/control/joystick_twist",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmTwistMapper::joystick_twist_callback, this, _1),
        joystick_twist_options
    );


    // Create timer and publisher for twist
    control_pub_timer = this->create_wall_timer(
        control_pub_timer_period, std::bind(&ArmTwistMapper::publish_control_inputs, this)
    );
    control_joints_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        "/control/control_joints", rclcpp::QoS(1).best_effort().deadline(200ms)
    );
    control_twist_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/control/control_twist", rclcpp::QoS(1).best_effort().deadline(200ms)
    );
    control_pose_pub = this->create_publisher<geometry_msgs::msg::TransformStamped>(
        "/control/control_pose", rclcpp::QoS(1).best_effort().deadline(200ms)
    );


    // Create service for arm_reset_control_pose
    arm_reset_control_pose_service = this->create_service<std_srvs::srv::Trigger>(
        "/control/arm_reset_control_pose", std::bind(&ArmTwistMapper::arm_reset_control_pose_callback, this, _1, _2)
    );


    // Initialise internal variables

    // Arm model and required solvers
    arm_model = new ArmModel(ArmConfig::wrist_type, ArmConfig::end_effector_type);
    arm_kinematics_solver = new ArmKinematics(*arm_model, this->get_logger());

    // Arrays in internal data structures
    // Use data from the arm model
    joints = ArmMessages::get_empty_joint_state(arm_model->joint_names);
    control_joints_msg = ArmMessages::get_empty_joint_state(arm_model->JOINT_NAMES_6DOF);

    // Control variables
    control_configuration = KDL::JntArray(arm_model->num_joints);
    prev_control_velocities = KDL::JntArray(arm_model->num_joints);
    // The control pose will not be correctly initialised here. Must reset once resolver data comes in
    control_pose = KDL::Frame::Identity();
    prev_control_twist = KDL::Twist::Zero();
    prev_time = this->now();


    // Output configuration messages
    // Convet module names to uppercase
    std::vector<std::string> module_names_upper = arm_model->module_names;
    for (auto& name : module_names_upper){
        for (auto& c : name){
            c = toupper(c);
        }
    }
    Print::title("ARM CONFIGURATION");
    Print::print("Wrist:");
    Print::print(module_names_upper[1].c_str(), 1);
    Print::print("End effector:");
    Print::print(module_names_upper[2].c_str(), 1);

    // Output set-up messages
    Print::title("ARM TWIST MAPPER");
    Print::print("Subscribed Topics:");
    Print::print("/control/arm_control_scheme           [core/ArmControlScheme]", 1);
    Print::print("/electronics/resolvers                [sensor_msgs/JointState]", 1);
    Print::print("/control/joystick_joint_velocities    [sensor_msgs/JointState]", 1);
    Print::print("/control/joystick_twist               [geometry_msgs/TwistStamped]", 1);
    Print::print("Published Topics:");
    Print::print("/control/control_joints               [sensor_msgs/JointState]", 1);
    Print::print("/control/control_twist                [geometry_msgs/TwistStamped]", 1);
    Print::print("/control/control_pose                 [geometry_msgs/TransformStamped]", 1);
    Print::print("Services:");
    Print::print("/control/arm_reset_control_pose       [std_srvs/Trigger]", 1);
    Print::print("", true);
}


// Update the internal control scheme
void ArmTwistMapper::control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg)
{
    // If rising edge on position control, reset current position and/or orientation
    // Also reset if swapping between task-space and joint-space
    if ((msg->position_control_linear && !control_scheme.position_control_linear)
        || msg->ik_linear != control_scheme.ik_linear) {
        reset_control_position();
    }
    if ((msg->position_control_angular && !control_scheme.position_control_angular)
        || msg->ik_angular != control_scheme.ik_angular) {
        reset_control_orientation();
    }
    control_scheme = *msg;
}


// Update the internal joint positions
void ArmTwistMapper::resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joints = *msg;
}


// Update the internal joint velocities
void ArmTwistMapper::joystick_joint_velocities_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    control_joints_msg.velocity = msg->velocity;
}
// Reset the internal velocity
void ArmTwistMapper::joystick_joint_velocities_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/joystick_joint_velocities subscription deadline missed");
    control_joints_msg.velocity = std::vector<double> (6);
}


// Update the internal task velocity
void ArmTwistMapper::joystick_twist_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg)
{
    joystick_twist = *msg;
}
// Reset the internal velocity
void ArmTwistMapper::joystick_twist_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/joystick_twist subscription deadline missed");
    joystick_twist = geometry_msgs::msg::TwistStamped();
}


// Map the input twist to the correct coordinate frame
inline KDL::Twist ArmTwistMapper::get_control_twist(const KDL::Twist& joystick_twist)
{
    // Unpack the ROS2 twist into KDL::Vectors
    KDL::Vector twist_linear = joystick_twist.vel;
    KDL::Vector twist_angular = joystick_twist.rot;

    // Endpoint frame control
    if (control_scheme.endpoint_frame_linear || control_scheme.endpoint_frame_angular){
        // Update the current end-effector orientation in the rover frame
        KDL::JntArray joint_positions = ArmTypeTranslation::to_KDL_jnt_array(joints.position);
        KDL::Rotation endpoint_coord_transform = arm_kinematics_solver->fk_pos_end_effector(joint_positions).M;
        // Transform from end effector coordinates to base frame coordinates
        // Inlcude input transforms to convert from joystick directions to intuitive end-effector frame coordinates
        if (control_scheme.endpoint_frame_linear) {
            twist_linear = endpoint_coord_transform * ENDPOINT_INPUT_TRANSFORM_LINEAR * twist_linear;
        }
        if (control_scheme.endpoint_frame_angular) {
            twist_angular = endpoint_coord_transform * ENDPOINT_INPUT_TRANSFORM_ANGULAR * twist_angular;
        }
    }

    // Reference frame offset
    if (control_scheme.base_frame_offset != 0){
        KDL::Rotation base_offset_transform = KDL::Rotation::RotZ(M_PI / 2 * control_scheme.base_frame_offset);
        // Apply offset only if endpoint-frame control not applied
        if (!control_scheme.endpoint_frame_linear){
            twist_linear = base_offset_transform * twist_linear;
        }
        if (!control_scheme.endpoint_frame_angular) {
            twist_angular = base_offset_transform * twist_angular;
        }
    }

    // Return the KDL twist
    return KDL::Twist(twist_linear, twist_angular);
}


// Integrate the joint-space control position up to the current time
inline void ArmTwistMapper::update_control_configuration(const KDL::JntArray& control_velocities, double timestep)
{
    bool update_lower_joints = !control_scheme.ik_linear && control_scheme.position_control_linear;
    bool update_wrist = !control_scheme.ik_angular && control_scheme.position_control_angular;
    if (update_lower_joints || update_wrist) {
        KDL::JntArray configuration_change;
        configuration_change.data = (prev_control_velocities.data + control_velocities.data) / 2 * timestep;

        // Calculate new configuration for lower joints
        if (update_lower_joints) {
            for (int i = 0; i < 3; i++) {
                control_configuration.data[i] += configuration_change.data[i];
                prev_control_velocities.data[i] = control_velocities.data[i];
            }
        }

        // Calculate new configuration for wrist
        if (update_wrist) {
            for (int i = 3; i < 6; i++) {
                control_configuration.data[i] += configuration_change.data[i];
                prev_control_velocities.data[i] = control_velocities.data[i];
            }
        }
    }
}


// Integrate the task-space control position up to the current time
inline void ArmTwistMapper::update_control_pose(const KDL::Twist& control_twist, double timestep)
{
    bool update_position = control_scheme.ik_linear && control_scheme.position_control_linear;
    bool update_orientation = control_scheme.ik_angular && control_scheme.position_control_angular;
    if (update_position || update_orientation) {
        KDL::Twist pose_change = (prev_control_twist + control_twist) / 2 * timestep;

        // Calculate new control position
        if (update_position) {
            control_pose.p += pose_change.vel;
            prev_control_twist.vel = control_twist.vel;
        }

        // Calculate new control orientation
        if (update_orientation) {
            double angle = pose_change.rot.Norm();
            control_pose.M = KDL::Rotation::Rot(pose_change.rot, angle) * control_pose.M;
            prev_control_twist.rot = control_twist.rot;
        }
    }
}


void ArmTwistMapper::publish_control_inputs()
{
    // Get the control joint velocities
    KDL::JntArray velocities = ArmTypeTranslation::to_KDL_jnt_array(control_joints_msg.velocity);
    // Get the control twist in the rover frame, depending on the control scheme
    KDL::Twist twist = get_control_twist(ArmTypeTranslation::to_KDL_twist(joystick_twist.twist));
    
    // Integrate to get the control pose and control configuration
    rclcpp::Time current_time = this->now();
    double timestep = (current_time - prev_time).seconds();
    update_control_configuration(velocities, timestep);
    update_control_pose(twist, timestep);
    prev_time = current_time;

    // Fill the output messages
    // control_joint_msg.velocity is already filled with the joystick velocities
    control_joints_msg.position = ArmTypeTranslation::to_std_vector(control_configuration);
    control_twist_msg.twist = ArmTypeTranslation::to_ROS2_twist(twist);
    control_pose_msg.transform = ArmTypeTranslation::to_ROS2_transform(control_pose);

    // Update the headers
    control_joints_msg.header.stamp = current_time;
    control_twist_msg.header.stamp = current_time;
    control_pose_msg.header.stamp = current_time;
    // Publish the messages
    control_joints_pub->publish(control_joints_msg);
    control_twist_pub->publish(control_twist_msg);
    control_pose_pub->publish(control_pose_msg);
}


// Set the control pose to the current position
void ArmTwistMapper::reset_control_position()
{
    // Get the joint-positions for the 6-DOF serial model of the arm
    KDL::JntArray joint_positions_6dof = ArmTypeTranslation::to_KDL_jnt_array(joints.position);
    joint_positions_6dof = arm_kinematics_solver->joint_positions_6dof(joint_positions_6dof);
    
    // Reset joint space
    for (int i = 0; i < 3; i++) {
        control_configuration.data[i] = joint_positions_6dof.data[i];
        prev_control_velocities.data[i] = 0;
    }

    // Reset task space
    control_pose.p = arm_kinematics_solver->fk_pos_end_effector_6dof(joint_positions_6dof).p;
    prev_control_twist.vel = KDL::Vector::Zero();

    // Print info message
    RCLCPP_INFO(this->get_logger(), "Position control reset (position / lower joints)");
}


// Set the control pose to the current orientation
void ArmTwistMapper::reset_control_orientation()
{
    // Get the joint-positions for the 6-DOF serial model of the arm
    KDL::JntArray joint_positions_6dof = ArmTypeTranslation::to_KDL_jnt_array(joints.position);
    joint_positions_6dof = arm_kinematics_solver->joint_positions_6dof(joint_positions_6dof);
    
    // Reset joint space
    for (int i = 3; i < 6; i++) {
        control_configuration.data[i] = joint_positions_6dof.data[i];
        prev_control_velocities.data[i] = 0;
    }

    // Reset task space
    control_pose.M = arm_kinematics_solver->fk_pos_end_effector_6dof(joint_positions_6dof).M;
    prev_control_twist.rot = KDL::Vector::Zero();

    // Print info message
    RCLCPP_INFO(this->get_logger(), "Position control reset (orientation / wrist)");
}


// Service to set the control pose to the current position
void ArmTwistMapper::arm_reset_control_pose_callback(
    __attribute__((unused)) const std_srvs::srv::Trigger::Request::SharedPtr request,
    std_srvs::srv::Trigger::Response::SharedPtr response
)
{
    reset_control_position();
    reset_control_orientation();
    response->success = true;
}



//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmTwistMapper>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
