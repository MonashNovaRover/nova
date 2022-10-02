#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class implements helper functions for converting
  between std, ROS2 and KDL types
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	   control
AUTHOR(S):   Jory Braun
CREATION:	   27/09/2022
EDITED:		   27/09/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS2 message types
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/transform.hpp"

// Inlcude KDL and related types
#include <kdl/frames.hpp>
#include <kdl/jntarray.hpp>

// Include other libraries
#include <vector>


namespace ArmTypeTranslation
{

    /// Convert std::vector to KDL::JntArray
    KDL::JntArray to_KDL_jnt_array(const std::vector<double>& joints);

    /// Convert ROS2 geometry_msgs::msg::Vector3 to KDL::Vector
    KDL::Vector to_KDL_vector(const geometry_msgs::msg::Vector3& vec3);

    /// Convert ROS2 geometry_msgs::msg::Twist to KDL::Vector
    KDL::Twist to_KDL_twist(const geometry_msgs::msg::Twist& twist);

    /// Convert ROS2 geometry_msgs::msg::Quaternion to KDL::Rotation
    KDL::Rotation to_KDL_rotation(const geometry_msgs::msg::Quaternion& rot);

    /// Convert ROS2 geometry_msgs::msg::Transform to KDL::Frame
    KDL::Frame to_KDL_frame(const geometry_msgs::msg::Transform& transform);

    /// Convert KDL::Vector to ROS2 geometry_msgs::msg::Vector3
    geometry_msgs::msg::Vector3 to_ROS2_vector(const KDL::Vector& kvec);

    /// Convert KDL::Twist to ROS2 geometry_msgs::msg::Twist
    geometry_msgs::msg::Twist to_ROS2_twist(const KDL::Twist& ktwist);

    /// Convert KDL::Rotation to ROS2 geometry_msgs::msg::Quaternion
    geometry_msgs::msg::Quaternion to_ROS2_quaternion(const KDL::Rotation& krot);

    /// Convert KDL::Frame to ROS2 geometry_msgs::msg::Transform
    geometry_msgs::msg::Transform to_ROS2_transform(const KDL::Frame& frame);

}

