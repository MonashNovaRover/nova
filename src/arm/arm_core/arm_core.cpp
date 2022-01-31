/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_core.h"


ArmCore::ArmCore(std::string node_name) : Node(node_name)
{
    // Empty for now. Just initialises the node
}

// Get empty JointState for use in other nodes and topics
sensor_msgs::msg::JointState ArmCore::get_empty_joint_state(const std::vector<std::string>& names)
{
    num_joints = names.size();
    
    sensor_msgs::msg::JointState msg;
    msg.name = names;
    msg.position = std::vector<double> (num_joints);
    msg.velocity = std::vector<double> (num_joints);
    msg.effort = std::vector<double> (num_joints);
    return msg;
}

// Get empty MultiDOFJointState for use in other nodes and topics
sensor_msgs::msg::MultiDOFJointState ArmCore::get_empty_multi_dof_joint_state(const std::vector<std::string>& names);
{
    num_joints = names.size();
    
    sensor_msgs::msg::MultiDOFJointState msg;
    msg.joint_names = names;
    msg.transforms = std::vector<geometry_msgs::msg::Transform> (num_joints);
    msg.twist = std::vector<geometry_msgs::msg::Twist> (num_joints);
    msg.wrench = std::vector<geometry_msgs::msg::Wrench> (num_joints);
    return msg;
}
