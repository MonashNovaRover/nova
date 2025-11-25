//
// Created by Bailey Chessum on 17/11/2025.
//

#ifndef ARM_KINEMATICS_EIGEN_FK_DETAIL_HPP
#define ARM_KINEMATICS_EIGEN_FK_DETAIL_HPP

#include <set>
#include <unordered_set>
#include <Eigen/Geometry>
#include <urdf_model/joint.h>
#include <urdf_model/pose.h>

#include "compute_joint_tree.hpp"
#include "compute_frame_tree.hpp"
#include "arm_kinematics/frame_definitions.hpp"

namespace arm_kinematics::detail {

inline Eigen::Isometry3d eigen_from_urdf_pose(const urdf::Pose & p)
{
  Eigen::Isometry3d T = Eigen::Isometry3d::Identity();
  T.translation() = Eigen::Vector3d(p.position.x, p.position.y, p.position.z);

  double x, y, z, w;
  p.rotation.getQuaternion(x, y, z, w);
  Eigen::Quaterniond q(w, x, y, z);
  T.linear() = q.toRotationMatrix();
  return T;
}

/**
 * For a requested frame (parent_link_name, origin),
 * walk *up* through fixed joints, accumulating them into offset,
 * and stop at:
 *   - the first REVOLUTE/PRISMATIC/CONTINUOUS joint (child link is base link), or
 *   - the URDF root (no parent_joint).
 *
 * Returns:
 *   base_link_name: link whose pose will come from ComputeJointTree::poses
 *   offset:         transform base_link -> requested frame
 *
 * NOTE:
 *   We do NOT collapse actuated joints into offset. They are handled by ComputeJointTree.
 */
struct BaseFrameInfo {
  std::string base_link_name;
  Eigen::Isometry3d offset;
};


inline BaseFrameInfo compute_base_frame(const urdf::ModelInterface & model,
                                        const std::string & root_link_name,
                                        const std::string & parent_link_name,
                                        const Eigen::Isometry3d & origin)
{
  urdf::LinkConstSharedPtr link = model.getLink(parent_link_name);
  if (!link) {
    throw std::runtime_error("Parent link '" + parent_link_name + "' not found in URDF");
  }

  // Current offset: link frame -> requested frame
  Eigen::Isometry3d offset = origin;

  while (true) {
    // If we reached the URDF root (no parent joint)
    if (!link->parent_joint) {
      // This link is effectively the root for this frame
      if (link->name != root_link_name) {
        throw std::runtime_error("Reached URDF root '" + link->name +
                                 "' before expected root '" + root_link_name + "'");
      }
      return BaseFrameInfo{link->name, offset};
    }

    urdf::JointConstSharedPtr pj = link->parent_joint;

    // If joint is actuated, stop HERE.
    // The base link is the *child* of this actuated joint (the link we’re currently on).
    if (pj->type == urdf::Joint::REVOLUTE ||
        pj->type == urdf::Joint::CONTINUOUS ||
        pj->type == urdf::Joint::PRISMATIC)
    {
      return BaseFrameInfo{link->name, offset};
    }

    // Otherwise, joint is FIXED: collapse it into offset and move to its parent link.
    if (pj->type != urdf::Joint::FIXED) {
      throw std::runtime_error("Unsupported joint type for FK mapping on joint '" + pj->name + "'");
    }

    urdf::LinkConstSharedPtr parent = model.getLink(pj->parent_link_name);
    if (!parent) {
      throw std::runtime_error("Parent link '" + pj->parent_link_name + "' not found");
    }

    // parent -> child transform at zero state
    Eigen::Isometry3d T_parent_child =
      eigen_from_urdf_pose(pj->parent_to_joint_origin_transform);

    // New offset: parent -> requested frame
    offset = T_parent_child * offset;
    link = parent;
  }
}


inline bool create_root_path(
  std::unordered_set<std::string> & root_path_name_set,
  std::vector<std::string> & root_path_names,
  const urdf::LinkConstSharedPtr & fake_root)
{
  if (!fake_root) {
    // Could not find the specified root!
    // TODO: Error message with rclcpp logging
    return false;
  }

  auto fake_root_parent = fake_root->getParent();
  while (fake_root_parent) {
    root_path_name_set.emplace(fake_root_parent->name);
    root_path_names.emplace_back(fake_root_parent->name);

    // Traverse to next parent
    fake_root_parent = fake_root_parent->getParent();
  }

  return true;
}

ComputeFrameTree build_fk_mapper_from_urdf(
    const urdf::ModelInterface & model,
    const std::string & root_link_name,
    FrameDefinitions frames,
    std::vector<std::string> & out_joint_names)
{


}

}

#endif //ARM_KINEMATICS_EIGEN_FK_DETAIL_HPP
