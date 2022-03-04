#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class defines the KDL model of the Equipment Servciing end effector
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 30/01/2022
EDITED:		 30/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Item One
 - Item Two
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_submodule.h"


class EeEquipmentServicingModel : public ArmSubModule
{
    //------------------------------------------------------------//
    public:

    // Parameters for arm model geometry. Based on model in Arm/DH parameters on GrabCAD
    // All distances in mm, all angles in rad
    constexpr static double GRIPPER_OFFSET = 0.210;

    /// Constructor. Build the equipment servicing end effector
    EeEquipmentServicingModel()
    {
        // Initialise public members
        // No joints
        endpoint_names = std::vector<std::string> {"gripper", "cam-front", "cam-depth", "cam-screw"};
        output_name = "sgripper";
        // No zero angles
        // No joint limits

        // Build the ES end effector

        // Gripper
        KDL::Joint gripper = KDL::Joint(endpoint_names[0], KDL::Joint::None);
        KDL::Frame fgripper = KDL::Frame::DH(0, 0, GRIPPER_OFFSET, 0);
        this->addSegment(KDL::Segment("sgripper", gripper, fgripper), "root");

        // Put all the cameras at the root for now, can move them to the correct spot later

        // Cam-front
        KDL::Joint cam_front = KDL::Joint(endpoint_names[1], KDL::Joint::None);
        KDL::Frame fcam_front = KDL::Frame::Identity();
        this->addSegment(KDL::Segment("scam_front", cam_front, fcam_front), "root");

        // Cam-depth
        KDL::Joint cam_depth = KDL::Joint(endpoint_names[1], KDL::Joint::None);
        KDL::Frame fcam_depth = KDL::Frame::Identity();
        this->addSegment(KDL::Segment("scam_depth", cam_depth, fcam_depth), "root");

        // Cam-screw
        KDL::Joint cam_screw = KDL::Joint(endpoint_names[1], KDL::Joint::None);
        KDL::Frame fcam_screw = KDL::Frame::Identity();
        this->addSegment(KDL::Segment("scam_screw", cam_screw, fcam_screw), "root");
    }

};