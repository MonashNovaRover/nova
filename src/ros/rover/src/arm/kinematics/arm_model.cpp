/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_model.h"
#include "arm_core.h"


ArmModel::ArmModel() : tree("root")
{
    // Build the arm.
    // Joints
    KDL::Joint J1 = KDL::Joint(ArmCore::joint_names[0], KDL::Joint::RotZ);  // Base rotation
    KDL::Joint J2 = KDL::Joint(ArmCore::joint_names[1], KDL::Joint::RotZ);  // Shoulder
    KDL::Joint J3 = KDL::Joint(ArmCore::joint_names[2], KDL::Joint::RotZ);  // Elbow
    KDL::Joint J4 = KDL::Joint(ArmCore::joint_names[3], KDL::Joint::RotZ);  // Wrist 1
    KDL::Joint J5 = KDL::Joint(ArmCore::joint_names[4], KDL::Joint::RotZ);  // Wrist 2
    KDL::Joint J6 = KDL::Joint(ArmCore::joint_names[5], KDL::Joint::RotZ);  // Wrist 3
    // End effectors
    KDL::Joint E0 = KDL::Joint("es-gripper", KDL::Joint::None);
    KDL::Joint E1 = KDL::Joint("lower_joints_hook", KDL::Joint::None);

    // Create transformations between joints
    // Each one is a transformation from the current joint to the previous one
    // Joints
    // Use the modified DH parameters, so the origin of frame i is at the output of joint i
    KDL::Frame FJ1 = KDL::Frame::DH_Craig1989(0, 0, 0, ArmCore::zero_angles[0]);
    KDL::Frame FJ2 = KDL::Frame::DH_Craig1989(0, M_PI / 2, ArmCore::SHOULDER_OFFSET, ArmCore::zero_angles[1]);
    KDL::Frame FJ3 = KDL::Frame::DH_Craig1989(ArmCore::ELBOW_LINK_LENGTH, 0, 0, ArmCore::zero_angles[2]);
    KDL::Frame FJ4 = KDL::Frame::DH_Craig1989(ArmCore::J4_LINK_LENGTH, 0, 0, ArmCore::zero_angles[3]);
    KDL::Frame FJ5 = KDL::Frame::DH_Craig1989(0, M_PI / -2, ArmCore::J5_OFFSET, ArmCore::zero_angles[4]);
    KDL::Frame FJ6 = KDL::Frame::DH_Craig1989(0, M_PI / -2, 0, ArmCore::zero_angles[5]);
    // End effectors
    // ES gripper
    KDL::Frame FE0 = KDL::Frame(KDL::Vector(0, 0, ArmCore::GRIPPER_OFFSET_Z));
    // Lower-joints hook
    KDL::Frame hook_to_j4 = KDL::Frame(KDL::Vector(ArmCore::HOOK_OFFSET_X, ArmCore::HOOK_OFFSET_Y, ArmCore::HOOK_OFFSET_Z));
    KDL::Rotation j4_to_elbow_rot = KDL::Rotation::Identity();
    j4_to_elbow_rot.DoRotZ(M_PI);
    j4_to_elbow_rot.DoRotY(-M_PI / 2);
    j4_to_elbow_rot.DoRotX(ArmCore::HOOK_ANGLE_X);
    // Check this rotation matrix.
    // Check j4_to_elbow_rot, make sure rotation part matches with transformation_j4_to_elbow in old model.py 
    KDL::Frame j4_to_elbow = KDL::Frame(j4_to_elbow_rot, KDL::Vector(ArmCore::J4_LINK_LENGTH, 0, 0));
    KDL::Frame FE1 = j4_to_elbow * hook_to_j4;

    // Create segments
    // Joints
    KDL::Segment SJ1 = KDL::Segment("SJ1", J1, FJ1);
    KDL::Segment SJ2 = KDL::Segment("SJ2", J2, FJ2);
    KDL::Segment SJ3 = KDL::Segment("SJ3", J3, FJ3);
    KDL::Segment SJ4 = KDL::Segment("SJ4", J4, FJ4);
    KDL::Segment SJ5 = KDL::Segment("SJ5", J5, FJ5);
    KDL::Segment SJ6 = KDL::Segment("SJ6", J6, FJ6);
    // End effectors
    KDL::Segment SE0 = KDL::Segment("SE0", E0, FE0);
    KDL::Segment SE1 = KDL::Segment("SE1", E1, FE1);

    // Add segments to the tree
    tree.addSegment(SJ1, "root");
    tree.addSegment(SJ2, "SJ1");
    tree.addSegment(SJ3, "SJ2");
    tree.addSegment(SJ4, "SJ3");
    tree.addSegment(SJ5, "SJ4");
    tree.addSegment(SJ6, "SJ5");
    tree.addSegment(SE0, "SJ6");
    tree.addSegment(SE1, "SJ3");
}