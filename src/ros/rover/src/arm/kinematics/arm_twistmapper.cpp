/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_twistmapper.h"

#include <Eigen/Core>

#include "arm_messages.h"
#include "../arm_configuration.h"
#include "print/print.h"

#define _USE_MATH_DEFINES
#include <cmath>
#include <string>

ArmTwistMapper::ArmTwistMapper() : Node("arm_twist_mapper")
{    
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
    task_velocity_timer_period = 50ms;
    task_velocity_timer = this->create_wall_timer(
        task_velocity_timer_period, std::bind(&ArmTwistMapper::publish_task_velocity, this)
    );
    task_velocity_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(
        "/control/task_velocity", rclcpp::QoS(1).best_effort().deadline(200ms)
    );

    // Initialise arm model and required solvers
    arm_model = new ArmModel(ArmConfig::wrist_type, ArmConfig::end_effector_type);
    spm_solver = new SpmKinematics();
    serial_fk_solver = new KDL::TreeFkSolverPos_recursive(*arm_model);

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


// Get the joint-space positions of the serial model of the arm
inline KDL::JntArray ArmTwistMapper::get_serial_joint_positions()
{
    KDL::JntArray kdl_joints;
    // Get the data directly from the resolvers
    kdl_joints.data = Eigen::Matrix<double, 6, 1> (joints.position.data());
    
    // If using the SPM wrist, replace SPM input joint positions with equivalent serial pitch, yaw and roll
    if (ArmConfig::wrist_type == ArmConfig::WRIST_SPM){
        // Calculate SPM FK
        std::vector<double> spm_joints (joints.position.begin() + 3, joints.position.begin() + 6); 
        std::vector<double> serial_wrist_joints = spm_solver->spm_fk(spm_joints);
        // Pitch
        kdl_joints.data[3] = serial_wrist_joints[0];
        // Yaw
        kdl_joints.data[4] = serial_wrist_joints[1];
        // Roll. Combine SPM roll and end-rotation since the KDL model requires 6 joints
        kdl_joints.data[5] = serial_wrist_joints[2] + joints.position[6];
    }

    return kdl_joints;
}


// Calculate the FK for a given segment
inline KDL::Frame ArmTwistMapper::calculate_serial_fk(KDL::JntArray kdl_joints, std::string segment_name)
{
    // Prepare the output data structure
    KDL::Frame kdl_coord_frame = KDL::Frame::Identity();
    
    // Calculate the FK for the given segment. Store the result in kdl_coord_frame
    int exit_value = serial_fk_solver->JntToCart(kdl_joints, kdl_coord_frame, segment_name);
    if (exit_value == -1){
        RCLCPP_WARN(this->get_logger(), "Number of positions provided does not match number of joints in tree");
    }
    else if (exit_value == -2){
        RCLCPP_WARN(this->get_logger(), "Could not find segment %s in the tree", segment_name.c_str());
    }

    return kdl_coord_frame;
}


// Get the twist from the joysticks
inline KDL::Twist ArmTwistMapper::get_control_twist()
{
    // Unpack the ROS2 task velocity into KDL::Vectors
    const geometry_msgs::msg::Vector3& vec3_linear = task_velocity.twist.linear;
    const geometry_msgs::msg::Vector3& vec3_angular = task_velocity.twist.angular;
    KDL::Vector twist_linear (vec3_linear.x, vec3_linear.y, vec3_linear.z);
    KDL::Vector twist_angular (vec3_angular.x, vec3_angular.y, vec3_angular.z);
    
    // Implement transformations on input linear and angular velocities
    // Endpoint frame control
    if (control_scheme.endpoint_frame_linear || control_scheme.endpoint_frame_angular){
        // Transform joystick input directions to end-effector coordinates
        // eg: forward on the left joystick is +ve x, but should be +ve z in end effector coordinates
        KDL::Rotation joystick_input_transform = KDL::Rotation::EulerZYX(M_PI / 2, -M_PI / 2, 0);
        // Transform from end effector coordinates to base frame coordinates
        KDL::Rotation endpoint_frame_transform = calculate_serial_fk(get_serial_joint_positions(), arm_model->default_endpoint_name).M;
        if (control_scheme.endpoint_frame_linear) {
            twist_linear = endpoint_frame_transform * joystick_input_transform * twist_linear;
        }
        if (control_scheme.endpoint_frame_angular) {
            // Add additional transform to switch yaw and roll directions for more intuitive control
            joystick_input_transform = KDL::Rotation::EulerZYX(0, 0, M_PI / 2) * joystick_input_transform;
            twist_angular = endpoint_frame_transform * joystick_input_transform * twist_angular;
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

    // Compose into final twist
    return KDL::Twist (twist_linear, twist_angular);
}


// Calculate the inverse kinematics using the latest arm model, publish to joint_velocities
void ArmTwistMapper::publish_task_velocity()
{
    // Get the twist in the rover frame
    KDL::Twist kdl_twist = get_control_twist();
    // Store the twist in the form ROS2 likes
    task_velocity.twist.linear.x = kdl_twist.vel.x();
    task_velocity.twist.linear.y = kdl_twist.vel.y();
    task_velocity.twist.linear.z = kdl_twist.vel.z();
    task_velocity.twist.angular.x = kdl_twist.rot.x();
    task_velocity.twist.angular.y = kdl_twist.rot.y();
    task_velocity.twist.angular.z = kdl_twist.rot.z();

    // Update the header
    task_velocity.header.stamp = this->now();
    // Publish the message
    task_velocity_pub->publish(task_velocity);
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
