/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_kinematics.h"

#include "../arm_configuration.h"
#include "print/print.h"

#include <string>

ArmKinematics::ArmKinematics(const ArmModel& arm_model, const rclcpp::Logger& logger) :
    arm_model(arm_model),
    serial_fk_solver(KDL::TreeFkSolverPos_recursive(arm_model)),
    serial_ik_solver(KDL::TreeIkSolverVel_wdls(arm_model, std::vector<std::string> {arm_model.default_endpoint_name})),
    spm_solver(SpmKinematics()),
    logger(logger)
{

}


// Get the joint-space positions of the serial model of the arm
inline KDL::JntArray ArmKinematics::serial_joint_positions(KDL::JntArray joint_positions)
{
    // Ensure the input is of the correct size
    if (joint_positions.data.size() != (int)arm_model.joint_names.size()) {
        // Throw an error, since if this is used in FK there is no safe default output
        RCLCPP_FATAL(logger, "Must provide 6 joint positions, or 7 joint positions with SPM");
        throw std::invalid_argument("Must provide 6 joint positions, or 7 joint positions with SPM");
    }

    // If using the SPM wrist, replace SPM input joint positions with equivalent serial pitch, yaw and roll
    if (ArmConfig::wrist_type == ArmConfig::WRIST_SPM) {
        // Calculate SPM FK
        std::vector<double> spm_joints (joint_positions.data.data() + 3, joint_positions.data.data() + 6); 
        std::vector<double> serial_wrist_joints = spm_solver.spm_fk(spm_joints);
        // Pitch
        joint_positions.data[3] = serial_wrist_joints[0];
        // Yaw
        joint_positions.data[4] = serial_wrist_joints[1];
        // Roll. Combine SPM roll and end-rotation since the KDL model requires 6 joints
        joint_positions.data[5] = serial_wrist_joints[2] + joint_positions.data[6];
        // Make output is 6-DOF
        joint_positions.data = joint_positions.data.head(6);
    }

    return joint_positions;
}

// Calculate the FK for a given segment
inline KDL::Frame ArmKinematics::serial_fk_pos_single_segment(KDL::JntArray serial_joint_positions, std::string segment_name)
{
    // Prepare the output data structure
    KDL::Frame frame = KDL::Frame::Identity();
    
    // Calculate the FK for the given segment
    int exit_value = serial_fk_solver.JntToCart(serial_joint_positions, frame, segment_name);
    if (exit_value == -1){
        RCLCPP_WARN(logger, "Number of positions provided does not match number of joints in tree");
    }
    else if (exit_value == -2){
        RCLCPP_WARN(logger, "Could not find segment %s in the tree", segment_name.c_str());
    }

    return frame;
}


// Calculate the FK for the end effector
KDL::Frame ArmKinematics::fk_pos_end_effector(KDL::JntArray joint_positions)
{
    return serial_fk_pos_single_segment(serial_joint_positions(joint_positions), arm_model.default_endpoint_name);
}


// Get the task-space positions of all coordinate frames on the arm using forward kinematics
std::vector<KDL::Frame> ArmKinematics::fk_pos_all_segments(KDL::JntArray joint_positions)
{
    // Get the input positions for the serial model of the arm, accounting for the SPM wrist
    joint_positions = serial_joint_positions(joint_positions);

    // Prepare the output data structure
    std::vector<KDL::Frame> frames;

    // Calculate FK for all joints
    // This is inefficient in KDL. For n joints takes O(n^2) time but could be O(n)
    for (std::size_t i = 0; i < arm_model.segment_names.size(); i++){
        // Calculate the FK for joint i
        frames.push_back(serial_fk_pos_single_segment(joint_positions, arm_model.segment_names[i]));
    }

    return frames;
}


// Solve the velocity inverse kineamtics for the end effector
inline KDL::JntArray ArmKinematics::serial_ik_vel_end_effector(KDL::JntArray serial_joint_positions, KDL::Twist twist)
{
    // Prepare the output data structure
    KDL::JntArray joint_velocities (6);
    
    if (twist != KDL::Twist::Zero()) {
        // Get the input twist in the form KDL likes
        KDL::Twists twists = { {arm_model.default_endpoint_name, twist} };

        // Calculate the inverse kinematics
        double exit_value = serial_ik_solver.CartToJnt(serial_joint_positions, twists, joint_velocities);
        if (exit_value == -1){
            RCLCPP_WARN(logger, "Must provide 6 positions and have 6 joints in tree");
        }
        else if (exit_value == -2){
            RCLCPP_WARN(logger, "Twists provided must have a corresponding endpoint which is a segment in the tree");
        }
        else if (exit_value == KDL::TreeIkSolverVel_wdls::E_SVD_FAILED) {
            RCLCPP_WARN(logger, "Singular value decomposition failed");
        }
    }

    return joint_velocities;
}


inline KDL::JntArray ArmKinematics::serial_to_actual_joint_vel_transform(KDL::JntArray joint_positions, KDL::JntArray joint_velocities, bool use_spm_roll)
{
    if (ArmConfig::wrist_type == ArmConfig::WRIST_SPM) {
        // Add another joint to the array
        joint_velocities.data << 0;

        // If using end rotation instead of SPM roll, move the serial roll to the index for end rotation
        // Then no roll will be passed to the SPM IK
        if (!use_spm_roll){
            joint_velocities.data[6] = joint_velocities.data[5];
            joint_velocities.data[5] = 0;
        }

        // Calculate SPM IK
        std::vector<double> spm_joints (joint_positions.data.data() + 3, joint_positions.data.data() + 6);
        std::vector<double> serial_wrist_velocity (joint_velocities.data.data() + 3, joint_velocities.data.data() + 6);
        std::vector<double> spm_velocity = spm_solver.spm_ik_velocity(spm_joints, serial_wrist_velocity);
        // Replace serial pitch, yaw and roll with SPM input joint velocities
        // Pitch
        joint_velocities.data[3] = spm_velocity[0];
        // Yaw
        joint_velocities.data[4] = spm_velocity[1];
        // Roll
        joint_velocities.data[5] = spm_velocity[2];
    }

    return joint_velocities;
}


// Solve the velocity inverse kinematics for the end effector, accounting for the SPM wrist
KDL::JntArray ArmKinematics::ik_vel_end_effector(KDL::JntArray joint_positions, KDL::Twist twist, bool use_spm_roll)
{
    // Calculate IK for the end effector
    // Gets the joint velocities for the serial model of the arm
    KDL::JntArray joint_velocities = serial_ik_vel_end_effector(serial_joint_positions(joint_positions), twist);

    // If using the SPM wrist, replace serial pitch, yaw and roll with SPM joint velocities
    joint_velocities = serial_to_actual_joint_vel_transform(joint_positions, joint_velocities, use_spm_roll);

    return joint_velocities;
}
