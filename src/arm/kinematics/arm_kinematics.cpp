/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_kinematics.h"

#include <Eigen/Core>

#include "arm_core.h"
#include "print/print.h"

#define _USE_MATH_DEFINES
#include <cmath>

ArmKinematics::ArmKinematics() : Node("arm_kinematics")
{
    // Initialise constants
    coord_frames_timer_period = 200ms;
    joint_velocities_timer_period = 200ms;

    // Initialise arm model
    wrist_type = ArmModel::WRIST_CYCLOIDAL;
    end_effector_type = ArmModel::EE_EQUIPMENT_SERVICING;
    arm_model = ArmModel(wrist_type, end_effector_type);
    // Initialise arm kinematics solvers
    arm_fk_solver = new KDL::TreeFkSolverPos_recursive(arm_model);
    arm_ik_solver = new KDL::TreeIkSolverVel_wdls(arm_model, std::vector<std::string> {arm_model.default_endpoint_name});

    // Initialise arrays in internal data structures
    // Use data from the arm model
    joints = ArmCore::get_empty_joint_state(arm_model.joint_names);
    // TwistStamped does not need to be initialised
    coord_frames = ArmCore::get_empty_multi_dof_joint_state(arm_model.segment_names);

    // Create subscription to arm control scheme
    control_scheme_sub = this->create_subscription<core::msg::ArmControlScheme>(
        "/control/arm_control_scheme", 10, std::bind(&ArmKinematics::control_scheme_callback, this, _1)
    );
    
    // Create subscription to resolvers
    resolver_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/electronics/resolvers", 10, std::bind(&ArmKinematics::resolver_callback, this, _1)
    );

    // Create subscription to task_velocity
    task_velocity_sub = this->create_subscription<geometry_msgs::msg::TwistStamped>(
        "/control/task_velocity", 10, std::bind(&ArmKinematics::task_velocity_callback, this, _1)
    );

    // Create timer and publisher for arm_coord_frames
    coord_frames_timer = this->create_wall_timer(
        coord_frames_timer_period, std::bind(&ArmKinematics::publish_coord_frames, this)
    );
    coord_frames_pub = this->create_publisher<sensor_msgs::msg::MultiDOFJointState>(
        "/control/arm_coord_frames", 10
    );

    // Create timer and publisher for joint_velocities
    joint_velocities_timer = this->create_wall_timer(
        joint_velocities_timer_period, std::bind(&ArmKinematics::publish_joint_velocities, this)
    );
    joint_velocities_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        "/control/joint_velocities_ik", 10
    );

    // Output set-up messages
    Print::title("ARM KINEMATICS");
    Print::print("Subscribed Topics:");
    Print::print("/electronics/resolvers      [sensor_msgs/JointState]", 1);
    Print::print("/control/task_velocity      [geometry_msgs/TwistStamped]", 1);
    Print::print("Published Topics:");
    Print::print("/control/arm_coord_frames   [sensor_msgs/MultiDOFJointState]", 1);
    Print::print("/control/joint_velocities   [sensor_msgs/JointState]", 1);
    Print::print("", true);
}


// Update the internal control scheme
void ArmKinematics::control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg)
{
    control_scheme = *msg;
}

// Update the internal joint state
void ArmKinematics::resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    joints = *msg;
}

// Update the internal velocity
void ArmKinematics::task_velocity_callback(const geometry_msgs::msg::TwistStamped::SharedPtr msg)
{
    task_velocity = *msg;
}

// Calculate the FK for a given segment
KDL::Frame ArmKinematics::calculate_fk(KDL::JntArray kdl_joints, std::string segment_name)
{
    // Prepare the output data structure
    KDL::Frame kdl_coord_frame;
    
    // Calculate the FK for the given segment. Store the result in kdl_coord_frame
    int exit_value = arm_fk_solver->JntToCart(kdl_joints, kdl_coord_frame, segment_name);
    if (exit_value == -1){
        RCLCPP_WARN(this->get_logger(), "Number of positions provided does not match number of joints in tree");
        return KDL::Frame::Identity();
    }
    else if (exit_value == -2){
        RCLCPP_WARN(this->get_logger(), "Could not find segment %s in the tree", segment_name.c_str());
        return KDL::Frame::Identity();
    }
    else{
        // Success
        return kdl_coord_frame;
    }
}

// Calculate the FK for a given segment
KDL::Frame ArmKinematics::calculate_fk(std::string segment_name)
{
    // Get the input positions in the form KDL likes
    KDL::JntArray kdl_joints;
    kdl_joints.data = Eigen::Matrix<double, 6, 1> (joints.position.data());

    return calculate_fk(kdl_joints, segment_name);
}

// Update the arm model using the latest resolver info, publish to arm_cord_frames
void ArmKinematics::publish_coord_frames()
{
    // Get the input positions in the form KDL likes
    KDL::JntArray kdl_joints;
    kdl_joints.data = Eigen::Matrix<double, 6, 1> (joints.position.data());
    
    // Calculate FK for all joints
    // This is inefficient in KDL. For n joints takes O(n^2) time but could be O(n)
    for (unsigned int i = 0; i < coord_frames.transforms.size(); i++){
        // Calculate the FK for joint i. Store the result in kdl_coord_frame
        KDL::Frame kdl_coord_frame = calculate_fk(kdl_joints, coord_frames.joint_names[i]);
        
        // Save the output transform in the form ROS2 likes
        geometry_msgs::msg::Vector3 translation;
        translation.x = kdl_coord_frame.p.x();
        translation.y = kdl_coord_frame.p.y();
        translation.z = kdl_coord_frame.p.z();
        geometry_msgs::msg::Quaternion rotation;
        kdl_coord_frame.M.GetQuaternion(rotation.x, rotation.y, rotation.z, rotation.w);
        geometry_msgs::msg::Transform transform;
        transform.translation = translation;
        transform.rotation = rotation;
        // Store the transform for this joint
        coord_frames.transforms[i] = transform;
    }

    // Update the header
    coord_frames.header.stamp = this->now();
    // Publish the message
    coord_frames_pub->publish(coord_frames);
}

// Calculate the inverse kinematics using the latest arm model, publish to joint_velocities
void ArmKinematics::publish_joint_velocities()
{
    // Clear the velocity data. Ensures if IK fails no velocity is sent to motors
    std::fill(joints.velocity.begin(), joints.velocity.end(), 0);
    
    // Get the input in the form KDL likes

    // Joint positions
    KDL::JntArray kdl_joint_positionss;
    kdl_joint_positionss.data = Eigen::Matrix<double, 6, 1> (joints.position.data());
    
    // Twist
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
        KDL::Rotation endpoint_frame_transform = calculate_fk(arm_model.default_endpoint_name).M;
        if (control_scheme.endpoint_frame_linear) {
            twist_linear = endpoint_frame_transform * joystick_input_transform * twist_linear;
        }
        if (control_scheme.endpoint_frame_angular) {
            // Add additional transform to switch yaw and roll directions for more intuitive control
            joystick_input_transform = KDL::Rotation::EulerZYX(0, 0, M_PI / 2) * joystick_input_transform;
            twist_angular = endpoint_frame_transform * joystick_input_transform * twist_angular;
        }
    }
    // Reference frame offset (if endpoint-frame control not applied)
    if (control_scheme.base_frame_offset != 0){
        KDL::Rotation base_offset_transform = KDL::Rotation::RotZ(M_PI / 2 * control_scheme.base_frame_offset);
        if (!control_scheme.endpoint_frame_linear){
            twist_linear = base_offset_transform * twist_linear;
        }
        if (!control_scheme.endpoint_frame_angular) {
            twist_angular = base_offset_transform * twist_angular;
        }
    }

    // Compose into final twist
    KDL::Twists kdl_twists { {arm_model.default_endpoint_name, KDL::Twist (twist_linear, twist_angular)} };
    
    // Prepare the output data structure
    KDL::JntArray kdl_joint_velocities;
    
    // Calculate the inverse kinematics
    double exit_value = arm_ik_solver->CartToJnt(kdl_joint_positionss, kdl_twists, kdl_joint_velocities);
    if (exit_value == -1){
        RCLCPP_WARN(this->get_logger(), "Must provide 6 positions and have 6 joints in tree");
    }
    else if (exit_value == -2){
        RCLCPP_WARN(this->get_logger(), "Twists provided must have a corresponding endpoint which is a segment in the tree");
    }
    else if (exit_value == KDL::TreeIkSolverVel_wdls::E_SVD_FAILED) {
        RCLCPP_WARN(this->get_logger(), "Singular value decomposition failed");
    }
    else{
        // Success
        // Save the output in the form ROS2 likes
        Eigen::Matrix<double, 6, 1> vec6 = kdl_joint_velocities.data;
        joints.velocity = std::vector<double> ( vec6.data(), vec6.data() + vec6.size() );
    }

    // Update the header
    joints.header.stamp = this->now();
    // Publish the message
    joint_velocities_pub->publish(joints);
}

//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmKinematics>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
