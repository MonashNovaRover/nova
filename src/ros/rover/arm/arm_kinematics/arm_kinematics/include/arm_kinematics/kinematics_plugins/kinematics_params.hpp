//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_KINEMATICS_PARAMS_HPP
#define ARM_KINEMATICS_KINEMATICS_PARAMS_HPP

#include <string>

namespace arm_kinematics {

/**
 * Parameters used to configure kinematics plugins
 */
struct KinematicsParams {

  /// The name of the link in the URDF to treat as the 'origin' for kinematics
  std::string base_link_name;

  /// The name of the link to use by default as the target in FK and IK calculations. Usually the end effector.
  std::string ee_link_name;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_KINEMATICS_PARAMS_HPP
