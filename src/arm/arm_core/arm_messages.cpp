/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_messages.h"


// Get empty JointState for use in other nodes and topics
sensor_msgs::msg::JointState ArmMessages::get_empty_joint_state(const std::vector<std::string>& names)
{
    std::vector<std::string>::size_type num_joints = names.size();
    
    sensor_msgs::msg::JointState msg;
    msg.name = names;
    msg.position = std::vector<double> (num_joints);
    msg.velocity = std::vector<double> (num_joints);
    msg.effort = std::vector<double> (num_joints);
    return msg;
}

// Get empty MultiDOFJointState for use in other nodes and topics
sensor_msgs::msg::MultiDOFJointState ArmMessages::get_empty_multi_dof_joint_state(const std::vector<std::string>& names)
{
    std::vector<std::string>::size_type num_joints = names.size();
    
    sensor_msgs::msg::MultiDOFJointState msg;
    msg.joint_names = names;
    msg.transforms = std::vector<geometry_msgs::msg::Transform> (num_joints);
    msg.twist = std::vector<geometry_msgs::msg::Twist> (num_joints);
    msg.wrench = std::vector<geometry_msgs::msg::Wrench> (num_joints);
    return msg;
}
