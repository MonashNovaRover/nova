//
// Created by Bailey Chessum on 15/10/2025.
//

#include <arm_kinematics/common/kinematics_params.hpp>
#include <arm_kinematics/utilities/param_reader.hpp>

namespace arm_kinematics {

const urdf::Model & KinematicsParams::get_urdf_model() const {
  if (!urdf_model_) {
    urdf_model_ = std::make_unique<urdf::Model>();
    urdf_model_->initString(robot_description);
  }

  return *urdf_model_;
}

KinematicsParams::KinematicsParams(const NodeParametersInterface::ConstSharedPtr & node,
  const std::string & robot_description): robot_description(robot_description) {
  const ParamReader params(node);

  base_link_name = params.get_or<std::string>("base_link_name", "base_link");
  ee_link_name = params.get_or<std::string>("base_link_name", "base_link");
}

} // arm_kinematics