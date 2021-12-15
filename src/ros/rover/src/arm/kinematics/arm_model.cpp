#include "arm_model.h"
#include <iostream>

ArmModel::ArmModel () : Node("arm_model")
{

    Eigen::Vector3d vec(1, 2, 3);
    std::cout << vec << std::endl;

    // Build the arm

    // Load params from config file
    // ArmParams params = GetParamsFromCoreArmParamsNode();


    // Create lower joints
    KDL::Joint J1 = KDL::Joint(KDL::Joint::RotZ);  // Base rotation
    KDL::Joint J2 = KDL::Joint(KDL::Joint::RotY);  // Shoulder
    KDL::Joint J3 = KDL::Joint(KDL::Joint::RotY);  // Elbow

    std::cout << KDL::Joint::RotZ << std::endl;
    
    // Construct the wrist
    // int wrist_select = params::wrist_type
    // Wrist wrist = wrist_select ? SpmWrist() : CycloidalWrist();
    
    // Joint J4 = Joint(Joint::RotY);  // Wrist Y. Real (for cycloidal) or modelled (for SPM)
    // Joint J5 = Joint(Joint::RotZ);  // Wrist Z. Real (for cycloidal) or modelled (for SPM)
    // Joint J6 = Joint(Joint::RotX);  // Wrist X. Real (for cycloidal) or modelled (for SPM)
    // Joint J7 = Joint(Joint::RotX);  // 
}

//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ArmModel>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}