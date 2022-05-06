#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class defines the KDL model of the SPM wrist

The model here described the SPM as an equivalent serial model, by parameterising the rotation of the
output plate relative to the input plate by Euler XYZ angles, corresponding to pitch, yaw and roll in that order.
This serial parameterisation is required since KDL does not support parallel manipulators.

A XYZ parameterisation is chosen (as opposed to the clasic ZYX) for a more intuitive control scheme when
commanding each angle independently. Starting with the X allows the wrist to cancel any angular elevation
due to the parallel shoulder or elbow joints, so decouples the yaw and roll from this varying elevation.
Ending with the Z means only pitch and yaw translate the camera view from the end of the end effector, so aids
in aligning to grasp objects. This parameterisation also matches the sequence of joints on the cycloidal wrist,
for which the ease of joint-space control has already been proven.

This model also includes the end rotation, which adds a redundant degree of freedom. The KDL IK solver requires
precisely 6 joints, so it is stored as a rigid joint for the purpose of KDLs kinematics solvers.
Here the XYZ parameterisation comes in useful as the end rotation can be modelled by combining it with the SPM roll.
When posing the arm using KDL's FK, the end rotation angle is simply added to the SPM roll angle.
Similarly, when solving IK, the resulting roll can be decomposed into SPM roll and end rotation roll.

For the purpose of constructing ROS messages to command motors, the end rotation is still listed as a joint.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 30/01/2022
EDITED:		 23/04/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Item One
 - Item Two
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_submodule.h"


class WristSpmModel : public ArmSubModule
{
    //------------------------------------------------------------//
    public:

    // Parameters for arm model geometry
    // All distances in mm, all angles in rad
    constexpr static double ROOT_BASE_LINK_LENGTH = 0.500;  // Update this value once SPM is attached to the arm
    constexpr static double CENTER_OFFSET = 0.09176;
    constexpr static double OUTPUT_OFFSET = 0.06373;
    
    /// Constructor. Build the SPM wrist
    WristSpmModel()
    {
        // Initialise public members
        module_name = "wrist_spm";
        joint_names = std::vector<std::string> {"spmx", "spmy", "spmz", "end-rotation"};
        // No endpoints
        output_name = "sjend";
        zero_angles = std::vector<double> {M_PI / 2, M_PI / 2, 0, 0};
        joint_limits = std::vector<JointLimit> {
            // Joint limits here apply to the SPM input angles, not the serial pitch, yaw and roll.
            // Implementation is in arm_kinematics.cpp
            {-2 * M_PI, 2 * M_PI},  // No joint limiting
            {-2 * M_PI, 2 * M_PI},  // No joint limiting
            {-2 * M_PI, 2 * M_PI},  // No joint limiting
            {-2 * M_PI, 2 * M_PI}  // No joint limiting
        };

        // Build the SPM wrist
        // Rigid link from root to the SPM base plate
        // Note this transformation requires two DH frames due to the non-standard definition of x-axes and origins
        KDL::Joint j3r2 = KDL::Joint("rigid-root-to-base", KDL::Joint::None);
        KDL::Frame fj3r2 = KDL::Frame::DH(ROOT_BASE_LINK_LENGTH, 0, 0, 0) * KDL::Frame::DH(0, -M_PI / 2, 0, -M_PI / 2);
        this->addSegment(KDL::Segment("sj3r2", j3r2, fj3r2), "root");
        
        // Rigid link from SPM base plate to SPM spherical centre
        KDL::Joint jspmr = KDL::Joint("rigid-base-to-center", KDL::Joint::None);
        KDL::Frame fjspmr = KDL::Frame::DH(0, M_PI / 2, CENTER_OFFSET, M_PI / 2);
        this->addSegment(KDL::Segment("sjspmr", jspmr, fjspmr), "sj3r2");

        // SPM pitch
        KDL::Joint jspmx = KDL::Joint(joint_names[0], KDL::Joint::RotZ);
        KDL::Frame fjspmx = KDL::Frame::DH(0, M_PI / 2, 0, zero_angles[0]);
        this->addSegment(KDL::Segment("sjspmx", jspmx, fjspmx), "sjspmr");
        
        // SPM yaw
        KDL::Joint jspmy = KDL::Joint(joint_names[1], KDL::Joint::RotZ);
        KDL::Frame fjspmy = KDL::Frame::DH(0, M_PI / 2, 0, zero_angles[1]);
        this->addSegment(KDL::Segment("sjspmy", jspmy, fjspmy), "sjspmx");

        // SPM roll
        KDL::Joint jspmz = KDL::Joint(joint_names[2], KDL::Joint::RotZ);
        KDL::Frame fjspmz = KDL::Frame::DH(0, 0, OUTPUT_OFFSET, zero_angles[2]);
        this->addSegment(KDL::Segment("sjspmz", jspmz, fjspmz), "sjspmy");

        // SPM end rotation
        // Store this joint separately to SPM roll so the wrist has all 4 joints,
        // but use a KDL::Joint::None so the IK solver only sees 6 joints
        KDL::Joint jend = KDL::Joint(joint_names[3], KDL::Joint::None);
        KDL::Frame fjend = KDL::Frame::DH(0, 0, 0, zero_angles[3]);
        this->addSegment(KDL::Segment("sjend", jend, fjend), "sjspmz");
    }

};