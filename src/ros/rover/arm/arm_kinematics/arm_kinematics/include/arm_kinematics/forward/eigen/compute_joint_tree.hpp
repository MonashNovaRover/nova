//
// Created by Bailey Chessum on 17/11/2025.
//

#ifndef ARM_KINEMATICS_COMPUTE_JOINT_TREE_HPP
#define ARM_KINEMATICS_COMPUTE_JOINT_TREE_HPP

#include <cstddef>
#include <Eigen/Geometry>
#include <arm_kinematics/aliases.hpp>
#include <rclcpp/logger.hpp>

#include "joint_type.hpp"

namespace arm_kinematics {

/**
 * Computes transforms of joint actuated links.
 *
 * \note This does not handle fixed joints, or fixed offset frames. Use a \c ComputeFrameTree instead for that purpose.
 */
class ComputeJointTree {
public:

  ComputeJointTree() = default;
  ComputeJointTree(std::vector<JointType> joint_types,
                std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d>> joint_axes,
                Isometry3dVector origins,
                std::vector<size_t> parents,
                const size_t root_relative_count)
      : joint_types_(std::move(joint_types)),
        joint_axes_(std::move(joint_axes)),
        origins_(std::move(origins)),
        parents_(std::move(parents)),
        root_relative_count_(root_relative_count)
  {
    assert(joint_types_.size() == joint_axes_.size());
    assert(joint_types_.size() >= parents_.size());
    assert(joint_types_.size() - root_relative_count_ == parents_.size());
    assert(joint_types_.size()  == origins_.size());
    poses = origins_;

    auto logger = rclcpp::get_logger("compute_joint_tree");
  }

  inline static void apply_joint(Eigen::Isometry3d & pose, const double state, const Eigen::Vector3d & axis, const JointType type) {
    switch (type) {
      case JointType::REVOLUTE:
      case JointType::CONTINUOUS: {
        const auto rotation = Eigen::AngleAxisd(state, axis);
        pose.linear() *= rotation.toRotationMatrix();
      }
      break;
      case JointType::PRISMATIC: {
        const Eigen::Vector3d translation = pose.linear() * axis * state;
        pose.translation().noalias() += translation;
      }
      break;
      default:
        break;
    }
  }

  void update(const std::vector<double> & joint_states) {
    // Calculate joints relative to the root
    for (size_t i = 0; i < root_relative_count_; ++i) {
      poses[i] = origins_[i];
      apply_joint(poses[i], joint_states[i], joint_axes_[i], joint_types_[i]);
    }

    // Calculate poses for non-root relative joints
    for (size_t i = root_relative_count_; i < poses.size(); ++i) {
      poses[i] = origins_[i];
      apply_joint(poses[i], joint_states[i], joint_axes_[i], joint_types_[i]);

      const auto & parent_pose = poses[parents_[i - root_relative_count_]];
      poses[i] = parent_pose * poses[i];
    }
  }

  [[nodiscard]] const Isometry3dVector & get_origins() const noexcept {
    return origins_;
  }
  [[nodiscard]] const Vector3dVector & get_axes() const noexcept {
    return joint_axes_;
  }
  [[nodiscard]] const std::vector<JointType> & get_types() const noexcept {
    return joint_types_;
  }
  [[nodiscard]] const std::vector<size_t> & get_parents() const noexcept {
    return parents_;
  }
  [[nodiscard]] const size_t & get_root_relative_count() const noexcept {
    return root_relative_count_;
  }

private:
  // Helpers
  // static JointType joint_type_from_urdf(const urdf::Joint & j);
  // static Eigen::Isometry3d eigen_from_urdf_pose(const urdf::Pose & p);

  /// Type for each joint
  std::vector<JointType> joint_types_{};

  /// The axis the joint actuates about in local space
  std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d>> joint_axes_{};

  /// Origin of each frame relative to the parent. Transforms from parent to child frame (when joint state is 0)
  Isometry3dVector origins_{};

  /// The index of the parent of all non-root joint links. Excludes those relative to the root! This is shorter than
  /// all the other vectors.
  std::vector<size_t> parents_{};

  /// The first N elements in the tree are relative to the root frame. This is that value of N.
  size_t root_relative_count_ = 0;

public:
  /**
   * All the poses of links in the tree,
   * with the invariant that for all i, poses[0...i-1] are independent of poses[i...poses.size()].
   * This invariant allows us to calculate each pose in order, using previous values in poses in calculations.
   */
  Isometry3dVector poses{}; //< Moved to bottom for correct initialization order
};

} // arm_kinematics

#endif //ARM_KINEMATICS_COMPUTE_JOINT_TREE_HPP