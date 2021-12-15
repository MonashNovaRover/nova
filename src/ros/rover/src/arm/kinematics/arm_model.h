#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class manages the simulated model of the arm, and
  performs associated kinematics calculations.
The class reads resolver data published to ROS and updates
  the arm model to match the real pose.
It reads the current task velocity and IK parameters
  published by the input node and publishes the required
  joint velocities.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: arm_model
TOPICS:
  - /control/resolvers         [Message Type]    [Subscribed]
  - /control/task_velocity     [Message Type]    [Subscribed]
  - /control/arm_coord_frames  [Message Type]    [Published]
  - /control/joint_velocities  [Message Type]    [Published]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S): Jory Braun
CREATION:	 11/12/2021
EDITED:		 11/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Item One
 - Item Two
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"

// Include libraries
#include <eigen3/Eigen/Dense>
#include <kdl/tree.hpp>

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;

/* 
Class which models the arm.
Use real positions of joints and end effectors, but idealised links
*/
class ArmModel : public rclcpp::Node {


    //------------------------------------------------------------//
    private:

    // Stores the loop timer for the update function
    rclcpp::TimerBase::SharedPtr timer;

    // Subscriber to resolvers

    // Subscriber to arm params

    // Subscriber to task velocity

    // Publisher to /control/arm_coord_frames

    // Publisher to /control/joint_velocities

    //------------------------------------------------------------//
    protected:

    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    ArmModel();
    
};