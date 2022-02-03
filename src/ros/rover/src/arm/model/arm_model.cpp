/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_model.h"

#include "lower_joints.h"
#include "wrist_cycloidal.h"
#include "wrist_spm.h"
#include "ee_equipment_servicing.h"
#include "ee_extreme_retrieval.h"
#include "ee_lunar_construction.h"

// Helper function for defining joint_names and control_point_names
// Is static to this file, is not part of ArmModel
static void append_to_vector(std::vector<std::string>& vec1, std::vector<std::string>& vec2)
{
   vec1.insert(vec1.end(), vec2.begin(), vec2.end());
}


ArmModel::ArmModel(WristType wrist_type, EndEffectorType end_effector_type)
{
    // Build the arm.

    // Create the lower joints module
    ArmSubModule lower_joints = LowerJointsModel();
    
    // Create the wrist module
    ArmSubModule wrist;
    switch (wrist_type){
        case WRIST_CYCLOIDAL:
            wrist = WristCycloidalModel();
        break;
        case WRIST_SPM:
            wrist = WristSpmModel();
    }
    
    // Crete the end effector module
    ArmSubModule end_effector;
    switch (end_effector_type){
        case EE_EQUIPMENT_SERVICING:
            end_effector = EeEquipmentServicingModel();
        break;
        case EE_EXTREME_RETRIEVAL:
            end_effector = EeExtremeRetrievalModel();
        break;
        case EE_LUNAR_CONSTRUCTION:
            end_effector = EeLunarConstructionModel();
    }

    // Add all the modules to the tree
    std::string attachment_name = "root";
    for (auto& module : std::vector<ArmSubModule> {lower_joints, wrist, end_effector} ) {
        // Add the Tree
        this->addTree(module, attachment_name);
        // Add the joint and control point names
        append_to_vector(joint_names, module.joint_names);
        append_to_vector(control_point_names, module.control_point_names);
        // Save the segment name where the next module attaches
        attachment_name = module.output_name;
    }
    
}