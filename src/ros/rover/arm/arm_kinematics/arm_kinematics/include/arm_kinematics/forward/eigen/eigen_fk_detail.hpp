//
// Created by Bailey Chessum on 17/11/2025.
//

#ifndef ARM_KINEMATICS_EIGEN_FK_DETAIL_HPP
#define ARM_KINEMATICS_EIGEN_FK_DETAIL_HPP

#include <unordered_set>
#include <Eigen/Geometry>
#include <urdf_model/joint.h>
#include <urdf_model/pose.h>

#include "eigen_fk_tree.hpp"
#include "eigen_fk_mapper.hpp"
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

inline EigenFKTree::JointType joint_type_from_urdf(const urdf::Joint & j)
{
  switch (j.type) {
    case urdf::Joint::REVOLUTE:
    case urdf::Joint::CONTINUOUS:
      return EigenFKTree::JointType::REVOLUTE;
    case urdf::Joint::PRISMATIC:
      return EigenFKTree::JointType::PRISMATIC;
    default:
      return EigenFKTree::JointType::FIXED;
  }
}

/**
 * For a requested frame specified by (parent_link_name, origin),
 * walk up through fixed joints, collapsing them into a single offset,
 * and stop at root_link or the first non-fixed joint.
 *
 * Returns:
 *   base_link_name: link whose pose we will pull from the FK tree
 *   offset:         transform base_link -> requested frame
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

  Eigen::Isometry3d offset = origin;  // current link -> frame

  while (true) {
    if (link->name == root_link_name) {
      return {link->name, offset};
    }

    urdf::JointConstSharedPtr pj = link->parent_joint;
    if (!pj) {
      throw std::runtime_error("Reached URDF root before '" + root_link_name + "'");
    }

    if (pj->type != urdf::Joint::FIXED) {
      // Stop at child of a non-fixed joint.
      return {link->name, offset};
    }

    // Collapse fixed joint into offset and move to parent
    urdf::LinkConstSharedPtr parent = model.getLink(pj->parent_link_name);
    if (!parent) {
      throw std::runtime_error("Parent link '" + pj->parent_link_name + "' not found");
    }

    Eigen::Isometry3d T_parent_child =
      eigen_from_urdf_pose(pj->parent_to_joint_origin_transform);

    offset = T_parent_child * offset;  // parent -> frame
    link = parent;
  }
}

EigenFKMapper build_fk_mapper_from_urdf(const urdf::ModelInterface & model,
                                        const std::string & root_link_name,
                                        FrameDefinitions frames,
                                        std::vector<std::string> & out_joint_names)
{
  const size_t M = frames.parent_link_names.size();

  // Compute all base links and offsets for requested frames
  std::vector<BaseFrameInfo> base_infos;
  base_infos.reserve(M);
  std::unordered_set<std::string> base_links_set;
  base_links_set.insert(root_link_name);

  for (size_t i = 0; i < M; ++i) {
    BaseFrameInfo info = compute_base_frame(model,
                                            root_link_name,
                                            frames.parent_link_names[i],
                                            frames.origins[i]);
    base_links_set.insert(info.base_link_name);
    base_infos.push_back(std::move(info));
  }

  // Build minimal link set for FK tree: paths root_link -> each base_link
  std::unordered_set<std::string> used_links;
  std::unordered_set<std::string> used_joints;
  used_links.insert(root_link_name);

  for (const std::string & name : base_links_set) {
    urdf::LinkConstSharedPtr link = model.getLink(name);
    if (!link) {
      throw std::runtime_error("Base link '" + name + "' not found in URDF");
    }

    urdf::LinkConstSharedPtr cur = link;
    while (cur && used_links.insert(cur->name).second) {
      if (cur->name == root_link_name)
        break;

      urdf::JointConstSharedPtr pj = cur->parent_joint;
      if (!pj)
        throw std::runtime_error("Link '" + cur->name + "' missing parent_joint");

      used_joints.insert(pj->name);

      urdf::LinkConstSharedPtr parent = model.getLink(pj->parent_link_name);
      if (!parent)
        throw std::runtime_error("Parent link '" + pj->parent_link_name + "' not found");

      cur = parent;
    }
  }

  // DFS to get topological order root -> leaves for this minimal subtree
  std::vector<urdf::LinkConstSharedPtr> ordered_links;
  ordered_links.reserve(used_links.size());

  std::function<void(const urdf::LinkConstSharedPtr &)> dfs =
    [&](const urdf::LinkConstSharedPtr & link)
  {
    if (!link) return;
    if (used_links.find(link->name) == used_links.end())
      return;

    ordered_links.push_back(link);

    for (const auto & cj : link->child_joints) {
      if (!cj) continue;
      if (used_joints.find(cj->name) == used_joints.end())
        continue;

      auto child = model.getLink(cj->child_link_name);
      dfs(child);
    }
  };

  urdf::LinkConstSharedPtr root_link = model.getLink(root_link_name);
  if (!root_link)
    throw std::runtime_error("Root link '" + root_link_name + "' not found");

  dfs(root_link);

  const size_t N = ordered_links.size();
  if (N == 0)
    throw std::runtime_error("FK tree is empty after URDF processing");

  // Build EigenFKTree input arrays
  std::vector<EigenFKTree::JointType> joint_types(N);
  std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d>> joint_axes(N);
  Isometry3dVector origins(N);
  size_t root_relative_count = 1;          // index 0 is root
  std::vector<size_t> parents(N - root_relative_count);

  std::vector<std::string> link_names(N);
  out_joint_names.clear();
  out_joint_names.reserve(N);

  std::unordered_map<std::string, size_t> link_to_index;
  link_to_index.reserve(N);

  for (size_t i = 0; i < N; ++i) {
    const auto & link = ordered_links[i];
    link_names[i] = link->name;
    link_to_index[link->name] = i;

    if (i == 0) {
      // Root
      joint_types[i] = EigenFKTree::JointType::FIXED;
      joint_axes[i].setZero();
      origins[i].setIdentity();
      out_joint_names[i].clear();
    } else {
      urdf::JointConstSharedPtr pj = link->parent_joint;
      if (!pj)
        throw std::runtime_error("Non-root link '" + link->name + "' missing parent_joint");

      joint_types[i] = joint_type_from_urdf(*pj);

      if (pj->type == urdf::Joint::REVOLUTE ||
          pj->type == urdf::Joint::CONTINUOUS ||
          pj->type == urdf::Joint::PRISMATIC)
      {
        joint_axes[i] = Eigen::Vector3d(pj->axis.x, pj->axis.y, pj->axis.z);
      } else {
        joint_axes[i].setZero();
      }

      origins[i] = eigen_from_urdf_pose(pj->parent_to_joint_origin_transform);
      out_joint_names[i] = pj->name;

      auto pit = link_to_index.find(pj->parent_link_name);
      if (pit == link_to_index.end())
        throw std::runtime_error("Parent link '" + pj->parent_link_name +
                                 "' not found while building parents");
      parents[i - root_relative_count] = pit->second;
    }
  }

  EigenFKTree tree(std::move(joint_types),
                   std::move(joint_axes),
                   std::move(origins),
                   std::move(parents),
                   root_relative_count);

  // Build mapping (tree_pose_indices, offsets) for requested frames
  std::vector<size_t> tree_pose_indices(M);
  Isometry3dVector offsets(M);

  for (size_t i = 0; i < M; ++i) {
    const auto & info = base_infos[i];
    auto it = link_to_index.find(info.base_link_name);
    if (it == link_to_index.end())
      throw std::runtime_error("Base link '" + info.base_link_name + "' not in FK tree");

    tree_pose_indices[i] = it->second;
    offsets[i] = info.offset;
  }

  // 6) Return fully constructed mapper; joint_names are passed through
  return EigenFKMapper(std::move(tree),
                       std::move(tree_pose_indices),
                       std::move(offsets));
}


}

#endif //ARM_KINEMATICS_EIGEN_FK_DETAIL_HPP
