//
// Created by Bailey Chessum on 17/11/2025.
//

#ifndef ARM_KINEMATICS_EIGENFKMAPPER_HPP
#define ARM_KINEMATICS_EIGENFKMAPPER_HPP

#include "eigen_fk_tree.hpp"

namespace arm_kinematics {

class EigenFKMapper {
public:
  EigenFKMapper(EigenFKTree tree,
                std::vector<size_t> tree_pose_indices,
                Isometry3dVector offsets)
    : tree_pose_indices_(std::move(tree_pose_indices)),
      offsets_(std::move(offsets)),
      tree_(std::move(tree)),
      varyings_(tree_pose_indices.size())
  {
    assert(tree_pose_indices_.size() == offsets_.size());
  }

  /// Joint states must match joint_names() order
  void update(const std::vector<double> & joint_states, Eigen::Isometry3d * data) {
    assert(data);

    tree_.update(joint_states);

    for (size_t i = 0; i < varyings_; ++i) {
      const size_t idx = tree_pose_indices_[i];
      data[i] = tree_.poses[idx] * offsets_[i];
    }
  }

private:
  /// Index of the pose in tree_ to use as the parent for the output pose at index i
  const std::vector<size_t> tree_pose_indices_;
  /// Final offset to apply in the output pose at index i.
  /// All the output poses we typically care about will have an offset from the closest parent joint that actuates.
  /// Probably std::move()-d from FrameDefinitions
  const Isometry3dVector offsets_;

  /// Used to do mapping for non-fixed joints
  EigenFKTree tree_;

  /// The number of non-constants
  const size_t varyings_;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_EIGENFKMAPPER_HPP