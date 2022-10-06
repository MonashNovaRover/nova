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
    KDL::TreeFkSolverPos_recursive fk_solver_6dof;
    KDL::TreeIkSolverVel_wdls ik_solver_6dof;
    SpmKinematics spm_solver;

    // Error logger
    rclcpp::Logger logger;


    /// @brief  Get the joint-space positions of the 6-DOF serial model of the arm
    ///         Return as a JntArray for use with KDL kinematics solvers
    KDL::JntArray joint_positions_6dof(KDL::JntArray joint_positions);

    /// @brief  Calculate the FK for a single segment using the 6-DOF serial model of the arm
    KDL::Frame fk_pos_single_segment_6dof(KDL::JntArray joint_positions_6dof, std::string segment_name);


    //------------------------------------------------------------//
    public:

    /// Constructor. Initialisers the solvers and starts the node
    ArmKinematics(const ArmModel& arm_model, const rclcpp::Logger& logger);

    /// @brief  Calculate the FK for the end effector
    KDL::Frame fk_pos_end_effector(KDL::JntArray joint_positions);

    /// @brief  Get the task-space positions of all coordinate frames on the arm using forward kinematics
    std::vector<KDL::Frame> fk_pos_all_segments(KDL::JntArray joint_positions);

    /// @brief  Calculate the IK for the end effector,using a 6-DOF serial joint model
    KDL::JntArray ik_vel_end_effector_6dof(KDL::JntArray joint_positions_6dof, KDL::Twist twist);

    /// @brief  Get the joint velocities for the joints on the arm
    KDL::JntArray joint_vel_transform_6dof_to_actual(KDL::JntArray joint_positions, KDL::JntArray joint_velocities, bool use_spm_roll=false);

    /// @brief  Get the joint-space velocities of all joints on the arm using inverse kinematics
    ///         Uses the current joint positions and desired task velocity, accoutns for SPM
    KDL::JntArray ik_vel_end_effector(KDL::JntArray joint_positions, KDL::Twist twist, bool use_spm_roll=false);
    
};