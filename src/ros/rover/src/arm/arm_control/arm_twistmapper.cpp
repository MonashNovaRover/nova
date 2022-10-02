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
    endpoint_input_transform_linear(KDL::Rotation::EulerZYX(M_PI / 2, -M_PI / 2, 0)),
    // Switch yaw and roll directions for more intuitive control
    endpoint_input_transform_angular(KDL::Rotation::RotX(M_PI / 2) * endpoint_input_transform_linear)
{    
    // Initialise publish timer periods
    task_inputs_timer_period = 50ms;
    
    
    // Create subscription to arm control scheme
    control_scheme_sub = this->create_subscription<core::msg::ArmControlScheme>(
        "/control/arm_control_scheme", 10, std::bind(&ArmTwistMapper::control_scheme_callback, this, _1)
    );

    // Create subscription to resolvers
    resolver_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/electronics/resolvers", 10, std::bind(&ArmTwistMapper::resolver_callback, this, _1)
    );
    
    // Create subscription to input_task_velocity
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> input_task_velocity_options;
    input_task_velocity_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void{
        this->input_task_velocity_deadline_callback();
    };
    input_task_velocity_sub = this->create_subscription<geometry_msgs::msg::TwistStamped>(
        "/control/input_task_velocity",
        rclcpp::QoS(1).best_effort().deadline(200ms),
        std::bind(&ArmTwistMapper::input_task_velocity_callback, this, _1),
        input_task_velocity_options
    );


    // Create timer and publisher for task_velocity
    task_inputs_timer = this->create_wall_timer(
        task_inputs_timer_period, std::bind(&ArmTwistMapper::publish_task_inputs, this)
    );
    task_velocity_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/control/task_velocity", rclcpp::QoS(1).best_effort().deadline(200ms)
    );
    task_position_pub = this->create_publisher<geometry_msgs::msg::TransformStamped>(
        "/control/task_position", rclcpp::QoS(1).best_effort().deadline(200ms)
    );


    // Initialise internal variables

    // Arm model and required solvers
    arm_model = new ArmModel(ArmConfig::wrist_type, ArmConfig::end_effector_type);
    arm_kinematics_solver = new ArmKinematics(*arm_model, this->get_logger());

    // Arrays in internal data structures
    // Use data from the arm model
    joints = ArmMessages::get_empty_joint_state(arm_model->joint_names);

    // Control variables
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
    Print::print("/control/arm_control_scheme       [core/ArmControlScheme]", 1);
    Print::print("/electronics/resolvers            [sensor_msgs/JointState]", 1);
    Print::print("/control/input_task_velocity      [geometry_msgs/TwistStamped]", 1);
    Print::print("Published Topics:");
    Print::print("/control/task_velocity            [geometry_msgs/TwistStamped]", 1);
    Print::print("/control/task_position            [geometry_msgs/TransformStamped]", 1);
    Print::print("", true);
}


// Update the internal control scheme
void ArmTwistMapper::control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg)
{
    control_scheme = *msg;
}


// Update the internal joint positions
void ArmTwistMapper::resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joints = *msg;
}


// Update the internal task velocity
void ArmTwistMapper::input_task_velocity_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg)
{
    input_task_velocity = *msg;
}
// Reset the internal velocity
void ArmTwistMapper::input_task_velocity_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "control/input_task_velocity subscription deadline missed");
    input_task_velocity = geometry_msgs::msg::TwistStamped();
}


// Map the input twist to the correct coordinate frame
inline KDL::Twist ArmTwistMapper::get_control_twist(const KDL::Rotation& endpoint_coord_transform)
{
    // Unpack the ROS2 twist into KDL::Vectors
    KDL::Twist twist = ArmTypeTranslation::to_KDL_twist(input_task_velocity.twist);
    KDL::Vector& twist_linear = twist.vel;
    KDL::Vector& twist_angular = twist.rot;

    // Endpoint frame control
    if (control_scheme.endpoint_frame_linear || control_scheme.endpoint_frame_angular){
        // Transform from end effector coordinates to base frame coordinates
        // Inlcude input transforms to convert from joystick directions to intuitive end-effector frame coordinates
        if (control_scheme.endpoint_frame_linear) {
            twist_linear = endpoint_coord_transform * endpoint_input_transform_linear * twist_linear;
        }
        if (control_scheme.endpoint_frame_angular) {
            twist_angular = endpoint_coord_transform * endpoint_input_transform_angular * twist_angular;
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
    return twist;
}


// Integrate the control position up to the current time
inline void ArmTwistMapper::update_control_pose(const KDL::Twist& control_twist, const KDL::Frame& endpoint_frame, double timestep)
{
    KDL::Twist pose_change;
    if (control_scheme.position_control_linear || control_scheme.position_control_angular) {
        pose_change = (prev_control_twist + control_twist) / 2 * timestep;
    }

    // Calculate new control position
    if (control_scheme.position_control_linear) {
        control_pose.p += pose_change.vel;
    }
    else {
        control_pose.p = endpoint_frame.p;
    }

    // Calculate new control orientation
    if (control_scheme.position_control_angular) {
        double angle = pose_change.rot.Norm();
        control_pose.M = KDL::Rotation::Rot(pose_change.rot, angle) * control_pose.M;
    }
    else {
        control_pose.M = endpoint_frame.M;
    }

    // Update previous state
    prev_control_twist = control_twist;
}


void ArmTwistMapper::publish_task_inputs()
{
    // Update the current end-effector pose in the rover frame
    KDL::JntArray joint_positions = ArmTypeTranslation::to_KDL_jnt_array(joints.position);
    KDL::Frame endpoint_frame_transform = arm_kinematics_solver->fk_pos_end_effector(joint_positions);

    // Get the control twist in the rover frame, depending on the control scheme
    KDL::Twist twist = get_control_twist(endpoint_frame_transform.M);
    // Integrate to get the control pose
    rclcpp::Time current_time = this->now();
    double timestep = (current_time - prev_time).seconds();
    update_control_pose(twist, endpoint_frame_transform, timestep);
    prev_time = current_time;

    // Fill the output messages
    task_velocity.twist = ArmTypeTranslation::to_ROS2_twist(twist);
    task_position.transform = ArmTypeTranslation::to_ROS2_transform(control_pose);

    // Update the headers
    task_velocity.header.stamp = current_time;
    task_position.header.stamp = current_time;
    // Publish the messages
    task_velocity_pub->publish(task_velocity);
    task_position_pub->publish(task_position);
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
