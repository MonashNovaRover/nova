/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "rviz_visualisation.h"

#include "print/print.h"


RvizVisualisation::RvizVisualisation() : Node("pose_array_publisher")
{

    // Create subscription to arm_coord_frames
    coord_frames_sub = this->create_subscription<sensor_msgs::msg::MultiDOFJointState>(
        "/control/arm_coord_frames", 10, std::bind(&RvizVisualisation::coord_frames_callback, this, _1)
    );

    // Create publisher for pose_array
    arm_poses_pub = this->create_publisher<geometry_msgs::msg::PoseArray>(
        "/control/arm_poses", 10
    );

    // Output set-up messages
    Print::title("RVIZ VISUALISATION");
    Print::print("Published Topics:");
    Print::print("/control/arm_poses   [geometry_msgs/PoseArray]", 1);
    Print::print("", true);

}

// Get data from the MultiDOFJointState message and re-publish as a PoseArray message
void RvizVisualisation::coord_frames_callback(const sensor_msgs::msg::MultiDOFJointState::SharedPtr frames_msg)
{
    // Construct the message. Create from scratch in each iteration since has no common info
    geometry_msgs::msg::PoseArray poses_msg;
    poses_msg.poses = std::vector<geometry_msgs::msg::Pose> (frames_msg->transforms.size());

    // Convert the data
    for (unsigned int i = 0; i < frames_msg->transforms.size(); i++){
        poses_msg.poses[i].position.x = frames_msg->transforms[i].translation.x / 1000;
        poses_msg.poses[i].position.y = frames_msg->transforms[i].translation.y / 1000;
        poses_msg.poses[i].position.z = frames_msg->transforms[i].translation.z / 1000;
        poses_msg.poses[i].orientation = frames_msg->transforms[i].rotation;
    }

    // Update the header
    poses_msg.header.stamp = this->now();
    poses_msg.header.frame_id = "map";
    // Publish the message
    arm_poses_pub->publish(poses_msg);
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