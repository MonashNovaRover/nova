#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class performs the forward and inverse kinematics
  calculations for the arm.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S): Jory Braun
CREATION:	 28/09/2022
EDITED:		 28/09/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"

// Include libraries
#include "arm_model.h"
#include "spm_kinematics.h"
#include <kdl/treefksolverpos_recursive.hpp>
#include <kdl/treeiksolvervel_wdls.hpp>


class ArmKinematics
{
    //------------------------------------------------------------//
    private:

    // Arm model and solvers
    ArmModel arm_model;
    KDL::TreeFkSolverPos_recursive serial_fk_solver;
    KDL::TreeIkSolverVel_wdls serial_ik_solver;
    SpmKinematics spm_solver;

    // Error logger
    rclcpp::Logger logger;


    /// @brief  Ensure the joint position input is of the correct size for the arm joints
    void validate_joint_size(KDL::JntArray joint_positions);

    /// @brief  Ensure hte joint position input is of the correct size for the serial arm model
    void validate_serial_joint_size(KDL::JntArray joint_positions);

    /// @brief  Get the joint-space positions of the serial model of the arm
    ///         Return as a JntArray for use with KDL kinematics solvers
    KDL::JntArray serial_joint_positions(KDL::JntArray joint_positions);

    /// @brief  Calculate the FK for a single segment using the serial model of the arm
    KDL::Frame serial_fk_pos_single_segment(KDL::JntArray serial_joint_positions, std::string segment_name);

    /// @brief  Calculate the IK for the end effector,using a serial joint model
    KDL::JntArray serial_ik_vel_end_effector(KDL::JntArray serial_joint_positions, KDL::Twist twist);


    //------------------------------------------------------------//
    public:

    /// Constructor. Initialisers the solvers and starts the node
    ArmKinematics(const ArmModel& arm_model, const rclcpp::Logger& logger);

    /// @brief  Calculate the FK for the end effector
    KDL::Frame fk_pos_end_effector(KDL::JntArray joint_positions);

    /// @brief  Get the task-space positions of all coordinate frames on the arm using forward kinematics
    std::vector<KDL::Frame> fk_pos_all_segments(KDL::JntArray joint_positions);

    /// @brief  Get the joint-space velocities of all joints on the arm using inverse kinematics
    ///         Uses the current joint positions and desired task velocity, accoutns for SPM
    KDL::JntArray ik_vel_end_effector(KDL::JntArray joint_positions, KDL::Twist twist, bool use_spm_roll=false);
    
};