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


ArmKinematics::ArmKinematics() : Node("arm_kinematics")
{
    // Initialise constants
    coord_frames_timer_period = 200ms;
    joint_velocities_timer_period = 200ms;

    // Initialise arm model and solvers
    arm_model = ArmModel();
    arm_fk_solver = new KDL::TreeFkSolverPos_recursive(arm_model);

    // Initialise arrays in internal data structures
    // Use data from the arm model
    joints = ArmCore::get_empty_joint_state(arm_model.joint_names);
    // TwistStamped does not need to be initialised
    coord_frames = ArmCore::get_empty_multi_dof_joint_state(arm_model.segment_names);

    // Create subscription to resolvers
    resolver_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/resolvers", 10, std::bind(&ArmKinematics::resolver_callback, this, _1)
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
        "/control/joint_velocities", 10
    );

    // Output set-up messages
    Print::title("ARM KINEMATICS");
    Print::print("Published Topics:");
    Print::print("/control/arm_coord_frames   [sensor_msgs/MultiDOFJointState]", 1);
    Print::print("/control/joint_velocities   [sensor_msgs/JointState]", 1);
    Print::print("", true);
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

// Update the arm model using the latest resolver info, publish to arm_cord_frames
void ArmKinematics::publish_coord_frames()
{
    // Get the input positions in the form KDL likes
    Eigen::Matrix<double, 6, 1> eigen_joints(joints.position.data());
    KDL::JntArray kdl_joints;
    kdl_joints.data = eigen_joints;
    // Prepare the output data structure
    KDL::Frame kdl_coord_frame;
    
    // Calculate FK for all joints
    // This is inefficient in KDL. For n joints takes O(n^2) time but could be O(n)
    for (unsigned int i = 0; i < coord_frames.transforms.size(); i++){
        // Calculate the FK for joint i. Store the result in kdl_coord_frame
        int exit_value = arm_fk_solver->JntToCart(
            kdl_joints, kdl_coord_frame, coord_frames.joint_names[i]
        );
        if (exit_value == -1){
            RCLCPP_WARN(this->get_logger(), "Number of positions provided does not match number of joints in tree");
        }
        else if (exit_value == -2){
            RCLCPP_WARN(this->get_logger(), "Could not find segment %s in the tree", coord_frames.joint_names[i].c_str());
        }
        else{
            // Success
            // Get the output transform in the form ROS2 likes
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
    }
    // Update the header
    coord_frames.header.stamp = this->now();
    // Publish the message
    coord_frames_pub->publish(coord_frames);
}

// Calculate the inverse kinematics using the latest arm model, publish to joint_velocities
void ArmKinematics::publish_joint_velocities()
{
    // Calculate the inverse kinematics
    //joints.velocity = MODEL.GET_THIS_BREAD(task_velocity);

    // Update the header
    joints.header.stamp = this->now();
    // Publish the message
    //joint_velocities_pub->publish(joints);
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