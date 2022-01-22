/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_core.h"

// Initialise non-const static members of ArmCore
std::vector<std::string> ArmCore::joint_names = {"base-rotation", "shoulder", "elbow", "wrist-1", "wrist-2", "wrist-3"};
std::vector<double> ArmCore::zero_angles = {0, M_PI / 2, -M_PI / 2, 0, -M_PI / 2, 0};

// Initialise empty_joint_state for use in other nodes and topics
sensor_msgs::msg::JointState ArmCore::get_empty_joint_state()
{
    sensor_msgs::msg::JointState joints;
    joints.name = ArmCore::joint_names;
    joints.position = ArmCore::zero_angles;
    joints.velocity = std::vector<double> (NUM_JOINTS);
    joints.effort = std::vector<double> (NUM_JOINTS);
    return joints;
}
