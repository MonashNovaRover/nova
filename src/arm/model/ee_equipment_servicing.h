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
#include "../arm_configuration.h"


class EeEquipmentServicingModel : public ArmSubModule
{
    //------------------------------------------------------------//
    public:
    
    // Parameters for arm model geometry
    // All distances in mm, all angles in rad
    constexpr static double GRIPPER_OFFSET = 0.210;
    // Parameters for wrist interfaces
    // Different values are used depending on what wrist is connected
    constexpr static double CYCLOIDAL_INTERFACE_OFFSET = 0;
    constexpr static double SPM_INTERFACE_OFFSET = 0;  // Update this value

    /// Constructor. Build the equipment servicing end effector
    EeEquipmentServicingModel(ArmConfig::WristType wrist_type)
    {
        // Initialise public members
        // No joints
        endpoint_names = std::vector<std::string> {"gripper", "cam-front", "cam-depth", "cam-screw"};
        output_name = "sgripper";
        // No zero angles
        // No joint limits

        // Build the ES end effector

        // Wrist interface
        double interface_offset;
        switch (wrist_type){
            case ArmConfig::WRIST_CYCLOIDAL:
                interface_offset = CYCLOIDAL_INTERFACE_OFFSET;
            break;
            case ArmConfig::WRIST_SPM:
                interface_offset = SPM_INTERFACE_OFFSET;
        }
        KDL::Joint interface = KDL::Joint("rigid-wirst-interface", KDL::Joint::None);
        KDL::Frame finterface = KDL::Frame::DH(0, 0, interface_offset, 0);
        this->addSegment(KDL::Segment("sinterface", interface, finterface), "root");

        // Gripper
        KDL::Joint gripper = KDL::Joint(endpoint_names[0], KDL::Joint::None);
        KDL::Frame fgripper = KDL::Frame::DH(0, 0, GRIPPER_OFFSET, 0);
        this->addSegment(KDL::Segment("sgripper", gripper, fgripper), "sinterface");

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