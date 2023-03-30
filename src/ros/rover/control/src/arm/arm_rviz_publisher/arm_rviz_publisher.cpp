/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_rviz_publisher.h"

#include <string>
#include "print/print.h"
#include "config/rosconfig.h"

// Standard namespace for subscribers
using std::placeholders::_1;

ArmVizPublisher::ArmVizPublisher() : Node("arm_rviz_publisher")
{

    // Create subscription to arm_coord_frames
    coord_frames_sub = this->create_subscription<sensor_msgs::msg::MultiDOFJointState>(
        "/control/arm_coord_frames", 10, std::bind(&ArmVizPublisher::coord_frames_callback, this, _1)
    );

    // Create subscription to arm control scheme
    control_scheme_sub = this->create_subscription<core::msg::ArmControlScheme>(
        "/control/arm_control_scheme", 10, std::bind(&ArmVizPublisher::control_scheme_callback, this, _1)
    );

    // Create subscription to control_pose
    control_pose_sub = this->create_subscription<geometry_msgs::msg::TransformStamped>(
        "/control/control_pose",
        rclcpp::QoS(1).best_effort().deadline(ROSTimers::arm_deadline),
        std::bind(&ArmVizPublisher::control_pose_callback, this, _1)
    );

    // Create publisher for arm_poses
    arm_poses_pub = this->create_publisher<geometry_msgs::msg::PoseArray>(
        "/control/arm_poses", 10
    );

    // Create publisher for arm_poses_path
    arm_path_pub = this->create_publisher<nav_msgs::msg::Path>(
        "/control/arm_poses_path", 10
    );

    // Create publisher for arm_control_pose
    arm_control_pose_pub = this->create_publisher<geometry_msgs::msg::PoseStamped>(
        "/control/arm_control_pose", 10
    );

    // Output set-up messages
    Print::title("ARM RVIZ PUBLISHER");
    Print::print("Subscribed Topics:");
    Print::print("/control/arm_coord_frames        [sensor_msgs/MultiDOFJointState]", 1);
    Print::print("/control/arm_control_scheme      [core/ArmControlScheme]", 1);
    Print::print("/control/control_pose            [geometry_msgs/TransformStamped]", 1);
    Print::print("Published Topics:");
    Print::print("/control/arm_poses               [geometry_msgs/PoseArray]", 1);
    Print::print("/control/arm_poses_path          [nav_msgs/Path]", 1);
    Print::print("/control/arm_control_pose        [geometry_msgs/PoseStamped]", 1);
    Print::print("", true);

}

// Get data from the MultiDOFJointState message and re-publish as PoseArray and Path messages
void ArmVizPublisher::coord_frames_callback(const sensor_msgs::msg::MultiDOFJointState::SharedPtr frames_msg)
{
    // Construct the PoseArray message
    geometry_msgs::msg::PoseArray poses_msg;
    poses_msg.poses = std::vector<geometry_msgs::msg::Pose> (frames_msg->transforms.size());

    // Construct the Path message
    nav_msgs::msg::Path path_msg;

    // Construct a standard header for all message types
    rclcpp::Time current_time = this->now();
    std_msgs::msg::Header header;
    header.stamp = current_time;
    header.frame_id = "map"; 
    
    // Convert the data from Transform types to Pose types
    // Store in the constructed messages
    for (unsigned int i = 0; i < frames_msg->transforms.size(); i++){
        
        // Fill the PoseArray message
        // Unpack Transform into Pose
        poses_msg.poses[i].position.x = frames_msg->transforms[i].translation.x;
        poses_msg.poses[i].position.y = frames_msg->transforms[i].translation.y;
        poses_msg.poses[i].position.z = frames_msg->transforms[i].translation.z;
        // Copy Quaternion
        poses_msg.poses[i].orientation = frames_msg->transforms[i].rotation;
        
        // Fill the Path message with the joints
        // Assumes the joints will all be in order and all start with "sj"
        if (frames_msg->joint_names[i].substr(0, 2) == "sj"){
            // Construct new PoseStamped entry
            geometry_msgs::msg::PoseStamped new_joint_pose;
            // Copy Pose
            new_joint_pose.pose = poses_msg.poses[i];
            // Add header
            new_joint_pose.header = header;
            // Add to message
            path_msg.poses.push_back(new_joint_pose);
        }

    }

    // Update the top-level headers
    poses_msg.header = header;
    path_msg.header = header;
    
    // Publish the messages
    arm_poses_pub->publish(poses_msg);
    arm_path_pub->publish(path_msg);

}


void ArmVizPublisher::control_scheme_callback(const core::msg::ArmControlScheme::SharedPtr msg)
{
    control_scheme = *msg;
}


void ArmVizPublisher::control_pose_callback(const geometry_msgs::msg::TransformStamped::SharedPtr msg)
{
    // Construct the PoseStamped message
    geometry_msgs::msg::PoseStamped pose;

    // Convert the data from TransformStamped to PoseStamped
    if (control_scheme.position_control) {
        // Unpack Transform into Pose
        pose.pose.position.x = msg->transform.translation.x;
        pose.pose.position.y = msg->transform.translation.y;
        pose.pose.position.z = msg->transform.translation.z;
        // Copy Quaternion
        pose.pose.orientation = msg->transform.rotation;
    }

    // Update the header
    pose.header.stamp = this->now();
    pose.header.frame_id = "map";

    // Publish the message
    arm_control_pose_pub->publish(pose);
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmVizPublisher>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
