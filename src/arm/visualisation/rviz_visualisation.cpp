/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "rviz_visualisation.h"

#include "print/print.h"


RvizVisualisation::RvizVisualisation() : Node("rviz_visualisation")
{

    // Create subscription to arm_coord_frames
    coord_frames_sub = this->create_subscription<sensor_msgs::msg::MultiDOFJointState>(
        "/control/arm_coord_frames", 10, std::bind(&RvizVisualisation::coord_frames_callback, this, _1)
    );

    // Create publisher for arm_poses
    arm_poses_pub = this->create_publisher<geometry_msgs::msg::PoseArray>(
        "/control/arm_poses", 10
    );

    // Create publisher for arm_poses_path
    arm_path_pub = this->create_publisher<nav_msgs::msg::Path>(
        "/control/arm_poses_path", 10
    );

    // Output set-up messages
    Print::title("RVIZ VISUALISATION");
    Print::print("Published Topics:");
    Print::print("/control/arm_poses         [geometry_msgs/PoseArray]", 1);
    Print::print("/control/arm_poses_path    [nav_msgs/Path]", 1);
    Print::print("", true);

}

// Get data from the MultiDOFJointState message and re-publish as PoseArray and Path messages
void RvizVisualisation::coord_frames_callback(const sensor_msgs::msg::MultiDOFJointState::SharedPtr frames_msg)
{
    // Construct the PoseArray message
    geometry_msgs::msg::PoseArray poses_msg;
    poses_msg.poses = std::vector<geometry_msgs::msg::Pose> (frames_msg->transforms.size());

    // Construct the Path message
    nav_msgs::msg::Path path_msg;
    path_msg.poses = std::vector<geometry_msgs::msg::PoseStamped> (frames_msg->transforms.size());

    // Construct a standard header for all message types
    rclcpp::Time current_time = this->now();
    std_msgs::msg::Header header;
    header.stamp = current_time;
    header.frame_id = "map"; 
    
    // Convert the data from Transform types to Pose types. Convert from mm to m
    // Store in the constructed messages
    for (unsigned int i = 0; i < frames_msg->transforms.size(); i++){
        
        // Fill the PoseArray message
        // Unpack Transform into Pose
        poses_msg.poses[i].position.x = frames_msg->transforms[i].translation.x / 1000;
        poses_msg.poses[i].position.y = frames_msg->transforms[i].translation.y / 1000;
        poses_msg.poses[i].position.z = frames_msg->transforms[i].translation.z / 1000;
        // Copy Quaternion
        poses_msg.poses[i].orientation = frames_msg->transforms[i].rotation;
        
        // Fill the Path message
        // Copy Pose
        path_msg.poses[i].pose = poses_msg.poses[i];
        // Add header
        path_msg.poses[i].header = header;

    }

    // Update the top-level headers
    poses_msg.header = header;
    path_msg.header = header;  
    
    // Publish the messages
    arm_poses_pub->publish(poses_msg);
    arm_path_pub->publish(path_msg);

}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<RvizVisualisation>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}