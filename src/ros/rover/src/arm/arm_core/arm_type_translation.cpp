/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_type_translation.h"
#include <Eigen/Core>


KDL::JntArray ArmTypeTranslation::to_KDL_jnt_array(const std::vector<double>& joints)
{
    KDL::JntArray kdl_joints (joints.size());
    for (std::size_t i = 0; i < joints.size(); i++) {
        kdl_joints.data[i] = joints[i];
    }
    return kdl_joints;
}

KDL::Vector ArmTypeTranslation::to_KDL_vector(const geometry_msgs::msg::Vector3& vec3)
{
    return KDL::Vector (vec3.x, vec3.y, vec3.z);
}

KDL::Twist ArmTypeTranslation::to_KDL_twist(const geometry_msgs::msg::Twist& twist)
{
    return KDL::Twist(to_KDL_vector(twist.linear), to_KDL_vector(twist.angular));
}

KDL::Rotation ArmTypeTranslation::to_KDL_rotation(const geometry_msgs::msg::Quaternion& rot)
{
    return KDL::Rotation::Quaternion(rot.x, rot.y, rot.z, rot.w);
}

KDL::Frame ArmTypeTranslation::to_KDL_frame(const geometry_msgs::msg::Transform& transform)
{
    return KDL::Frame(to_KDL_rotation(transform.rotation), to_KDL_vector(transform.translation));
}

std::vector<double> ArmTypeTranslation::to_std_vector(const KDL::JntArray& joints)
{
    return std::vector<double> (joints.data.data(), joints.data.data() + joints.data.size());
}

geometry_msgs::msg::Vector3 ArmTypeTranslation::to_ROS2_vector(const KDL::Vector& kvec)
{
    geometry_msgs::msg::Vector3 vec3;
    vec3.x = kvec.x();
    vec3.y = kvec.y();
    vec3.z = kvec.z();
    return vec3;
}

geometry_msgs::msg::Twist ArmTypeTranslation::to_ROS2_twist(const KDL::Twist& ktwist)
{
    geometry_msgs::msg::Twist twist;
    twist.linear = to_ROS2_vector(ktwist.vel);
    twist.angular = to_ROS2_vector(ktwist.rot);
    return twist;
}

geometry_msgs::msg::Quaternion ArmTypeTranslation::to_ROS2_quaternion(const KDL::Rotation& krot)
{
    geometry_msgs::msg::Quaternion rot;
    krot.GetQuaternion(rot.x, rot.y, rot.z, rot.w);
    return rot;
}

geometry_msgs::msg::Transform ArmTypeTranslation::to_ROS2_transform(const KDL::Frame& frame)
{
    geometry_msgs::msg::Transform transform;
    transform.translation = to_ROS2_vector(frame.p);
    transform.rotation = to_ROS2_quaternion(frame.M);
    return transform;
}
