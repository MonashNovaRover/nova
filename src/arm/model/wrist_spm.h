#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class defines the KDL model of the SPM wrist
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
    constexpr static double CENTER_OFFSET = 0.04082;
    constexpr static double OUTPUT_OFFSET = 0.04082;  // Check this value. Should account for end rotation

    constexpr static double ROOT_J4_LINK_LENGTH = 0.499;
    constexpr static double J4_OFFSET = 0.09952;
    constexpr static double J5_OFFSET = 0.1041;
    constexpr static double J6_OFFSET = 0.1079;
    
    /// Constructor. Build the SPM wrist
    WristSpmModel()
    {
        // Initialise public members
        joint_names = std::vector<std::string> {"spmz", "spmy", "spmx", "end-rotation"};
        // No endpoints
        output_name = "sjend";
        zero_angles = std::vector<double> {0, -M_PI / 2, -M_PI / 2, -M_PI / 2};
        joint_limits = std::vector<JointLimit> {
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
        
        // SPM roll
        // Also includes the translation from the base plate to the center of rotation
        KDL::Joint jspmz = KDL::Joint(joint_names[0], KDL::Joint::RotZ);
        KDL::Frame fjspmz = KDL::Frame::DH(0, -M_PI / 2, CENTER_OFFSET, zero_angles[0]);
        this->addSegment(KDL::Segment("sjspmz", jspmz, fjspmz), "sj3r2");

        // SPM yaw
        KDL::Joint jspmy = KDL::Joint(joint_names[1], KDL::Joint::RotZ);
        KDL::Frame fjspmy = KDL::Frame::DH(0, -M_PI / 2, 0, zero_angles[1]);
        this->addSegment(KDL::Segment("sjspmy", jspmy, fjspmy), "sjspmz");

        // SPM pitch
        KDL::Joint jspmx = KDL::Joint(joint_names[2], KDL::Joint::RotZ);
        KDL::Frame fjspmx = KDL::Frame::DH(0, -M_PI / 2, 0, zero_angles[2]);
        this->addSegment(KDL::Segment("sjspmx", jspmx, fjspmx), "sjspmy");

        // End rotation
        KDL::Joint jend = KDL::Joint(joint_names[3], KDL::Joint::RotZ);
        KDL::Frame fjend = KDL::Frame::DH(0, 0, OUTPUT_OFFSET, zero_angles[3]);
        this->addSegment(KDL::Segment("sjend", jend, fjend), "sjspmx");
    }

};