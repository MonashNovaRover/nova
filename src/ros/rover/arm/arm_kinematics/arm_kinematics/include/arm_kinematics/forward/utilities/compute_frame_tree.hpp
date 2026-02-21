//
// Created by Bailey Chessum on 17/11/2025.
//

#ifndef ARM_KINEMATICS_COMPUTE_FRAME_TREE_HPP
#define ARM_KINEMATICS_COMPUTE_FRAME_TREE_HPP

#include <queue>

#include "arm_kinematics/visibility_control.h"
#include "compute_joint_tree.hpp"
#include "arm_kinematics/utilities/aliases.hpp"

namespace arm_kinematics {

/**
 * Computes transforms of any frames (that aren't relative to the root frame) by applying fixed offsets from a given
 * \c ComputeJointTree 's poses.
 *
 * \note To handle fixed links/frames relative to the root, just use a Isometry3fVector larger than your
 * \c ComputeFrameTree needs, and use the values that aren't modified by the tree for these links/frames, setting their
 * value once. They will never change!
 *
 * \see ComputeJointTree
 */
class ARM_KINEMATICS_PUBLIC ComputeFrameTree {
public:
  ComputeFrameTree(ComputeJointTree tree,
                std::vector<size_t> tree_pose_indices,
                Isometry3fVector offsets)
    : tree_pose_indices_(std::move(tree_pose_indices)),
      offsets_(std::move(offsets)),
      tree_(std::move(tree)),
      varyings_(tree_pose_indices.size())
  {
    assert(tree_pose_indices_.size() <= offsets_.size());
  }

  ComputeFrameTree() = default;

  /// Joint states must match joint_names() order
  void update(const std::vector<float> & joint_states, Eigen::Isometry3f * data) {
    assert(data);

    tree_.update(joint_states);

    for (size_t i = 0; i < tree_pose_indices_.size(); ++i) {
      const size_t idx = tree_pose_indices_[i];
      data[i] = tree_.poses[idx] * offsets_[i];
    }

    // TODO: We have to do the below for correctness for constant poses. But could we just assign this once?
    for (size_t i = tree_pose_indices_.size(); i < offsets_.size(); ++i)
      data[i] = offsets_[i];
  }

  [[nodiscard]] const Isometry3fVector & get_origins() const noexcept { return offsets_; }
  [[nodiscard]] const std::vector<size_t> & get_parents() const noexcept { return tree_pose_indices_; }
  [[nodiscard]] const ComputeJointTree & get_tree() const noexcept { return tree_; }  //< For access in tests

private:
  /// Index of the pose in tree_ to use as the parent for the output pose at index i
  std::vector<size_t> tree_pose_indices_;
  /// Final offset to apply in the output pose at index i.
  /// All the output poses we typically care about will have an offset from the closest parent joint that actuates.
  /// Probably std::move()-d from FrameDefinitions
  Isometry3fVector offsets_{};

  /// Used to do mapping for non-fixed joints
  ComputeJointTree tree_{};

  /// The number of non-constants
  size_t varyings_ = 0;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_COMPUTE_FRAME_TREE_HPP