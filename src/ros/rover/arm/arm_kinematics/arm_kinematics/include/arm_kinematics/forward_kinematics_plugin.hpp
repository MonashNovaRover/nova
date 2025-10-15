//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef BANKSIA_KINEMATICS_PLUGIN_KINEMATICS_PLUGIN_BASE_HPP
#define BANKSIA_KINEMATICS_PLUGIN_KINEMATICS_PLUGIN_BASE_HPP

#include "visibility_control.h"
#include <string>
#include <vector>
#include <geometry_msgs/msg/pose.hpp>
#include <Eigen/Geometry>
#include <optional>
#include <urdf/model.h>
#include <kdl_parser/kdl_parser.hpp>
#include <kdl/jntarray.hpp>
#include "kinematics_base.hpp"


namespace arm_kinematics {

/**
 * Plugin class used to perform forward kinematics.
 *
 * Has a good default implementation, but affords modification/extension/optimisation through pluginlib.
 */
class ARM_KINEMATICS_PUBLIC ForwardKinematicsPlugin : public KinematicsBase {
public:
  using SharedPtr = std::shared_ptr<ForwardKinematicsPlugin>;

  /**
   * Effectively replaces the constructor for the class, as we can only use a default constructor in plugins.
   *
   * Very expensive, and obviously not real-time safe.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  bool initialize(
    KinematicsNodeInterfaces node_interfaces,
    std::string & robot_description,
    const std::vector<std::string>& joint_names);


  /**
   * Do Forward Kinematics to find the position of a link with the given name.
   *
   * \param[in] joint_angles The current angle of every joint in get_joint_names(), in radians.
   * \param[in] link_name The name of the link to find the pose for.
   * \param[out] solution_pose The pose found through forward kinematics.
   *
   * \returns True if a solution could be found. False otherwise.
   */
  virtual bool get_position_fk(
    const std::vector<double> & joint_angles,
    const std::string & link_name,
    Eigen::Isometry3d & solution_pose) const;

  /**
    * Do Forward Kinematics to find the position of the main End Effector link.
    *
    * \param[in] joint_angles The current angle of every joint in get_joint_names(), in radians.
    * \param[out] solution_pose The pose found through forward kinematics.
    *
    * \returns True if a solution could be found. False otherwise.
    */
  virtual bool get_position_fk(
    const std::vector<double> & joint_angles,
    Eigen::Isometry3d & solution_pose) const;;

  // Accessors


protected:
  /**
   * Called when the kinematics plugin is created. Override this to add any set up logic for the kinematics plugin.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize() = 0;

private:

  // KDL
  urdf::Model urdf_model_;
  KDL::Tree kdl_tree_;
  /// The default kdl chain to use, from the base link to the ee link
  KDL::Chain kdl_chain_;
  /// Pre-allocated KDL::JntArray to use for per-joint values
  KDL::JntArray preallocated_jnts{};
};

} // arm_kinematics

#endif //BANKSIA_KINEMATICS_PLUGIN_KINEMATICS_PLUGIN_BASE_HPP
