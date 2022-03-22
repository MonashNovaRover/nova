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

// Helper function for defining joint_names and endpoint_names
// Is static to this file, is not part of ArmModel
template<typename T>
static void append_to_vector(std::vector<T>& vec1, std::vector<T>& vec2)
{
   vec1.insert(vec1.end(), vec2.begin(), vec2.end());
}


ArmModel::ArmModel(WristType wrist_type, EndEffectorType end_effector_type) : Tree("sj0")
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
    // Track where the next module should attach, starting at the current root
    std::string output_name = this->getRootSegment()->first;
    for (auto& module : std::vector<ArmSubModule> {lower_joints, wrist, end_effector} ) {
        // Add the Tree
        this->addTree(module, output_name);
        // Add the joint and control point names
        append_to_vector<std::string>(joint_names, module.joint_names);
        append_to_vector<std::string>(endpoint_names, module.endpoint_names);
        // Add the joint limits
        append_to_vector<ArmSubModule::JointLimit>(joint_limits, module.joint_limits);
        // Save the segment name where the next module attaches
        output_name = module.output_name;
    }

    // Set the default endpoint
    default_endpoint_name = output_name;

    // Construct list of segment names for the entire arm
    for (auto const& segment_pair : this->getSegments()){
        segment_names.push_back(segment_pair.first);
    }
}