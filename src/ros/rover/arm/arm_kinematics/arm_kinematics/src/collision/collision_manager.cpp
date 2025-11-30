//
// Created by nova on 11/30/25.
//

#include <arm_kinematics/collision/collision_manager.hpp>
#include <arm_kinematics/utilities/to_eigen.hpp>

namespace arm_kinematics {

CollisionManager::MakeCollisionResult::MakeCollisionResult(
  const std::vector<std::string> & joint_names,
  PluginLoader& loader,
  const ForwardKinematicsPlugin::SharedPtr& fk)
{
}

CollisionManager::CollisionManager(
  const std::vector<std::string> & joint_names,
  PluginLoader & loader,
  const ForwardKinematicsPlugin::SharedPtr & fk)
: data(joint_names, loader, fk)
{
}

} // arm_kinematics