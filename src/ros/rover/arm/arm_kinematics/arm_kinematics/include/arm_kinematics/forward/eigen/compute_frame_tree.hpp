//
// Created by Bailey Chessum on 17/11/2025.
//

#ifndef ARM_KINEMATICS_EIGENFKMAPPER_HPP
#define ARM_KINEMATICS_EIGENFKMAPPER_HPP

#include <queue>

#include "analysis_tree.hpp"
#include "compute_joint_tree.hpp"

namespace arm_kinematics {

/**
 * Computes transforms of any frames (that aren't relative to the root frame) by applying fixed offsets from a given
 * \c ComputeJointTree 's poses.
 *
 * \note To handle fixed links/frames relative to the root, just use a Isometry3dVector larger than your
 * \c ComputeFrameTree needs, and use the values that aren't modified by the tree for these links/frames, setting their
 * value once. They will never change!
 *
 * \see ComputeJointTree
 */
class ComputeFrameTree {
public:
  ComputeFrameTree(ComputeJointTree tree,
                std::vector<size_t> tree_pose_indices,
                Isometry3dVector offsets)
    : tree_pose_indices_(std::move(tree_pose_indices)),
      offsets_(std::move(offsets)),
      tree_(std::move(tree)),
      varyings_(tree_pose_indices.size())
  {
    assert(tree_pose_indices_.size() == offsets_.size());
  }

  ComputeFrameTree(
    const detail::AnalysisTree & analysis,
    const std::string & root_name,
    const FrameDefinitions & definitions)
  {
    const auto & joints = analysis.get_joints();
    const auto & frames = analysis.get_frames();

    // indexes of frames for each definition
    std::vector<size_t> definition_frame_ids(definitions.size());
    for (size_t i = 0; i < frames.size(); ++i) {
      definition_frame_ids[i] = frames[definitions.parent_link_names[i]];
    }

    // First figure out root
    const auto root_frame_id = frames.get(root_name);
    if (!root_frame_id.has_value())
      throw std::invalid_argument("Given root_name is not in the AnalysisTree");
    const auto root_joint_id = frames[*root_frame_id].parent;

    // Find reversed path
    std::vector<bool> reversed_mask(joints.size(), false);
    size_t current = root_joint_id;
    while (current != 0) {
      reversed_mask[current] = true;
      current = joints[current].parent;
    }
    reversed_mask[0] = true;

    // First find out what is in the subtree
    std::vector<bool> subtree_mask(joints.size(), false);

    // O(definitions.size() + subtree size)
    size_t reversed_path_end = root_joint_id;
    for (const auto & frame_id : definition_frame_ids) {
      current = frames[frame_id].parent;  //< current joint id

      while (!subtree_mask[current]) {
        subtree_mask[current] = true; //< Don't traverse the same place twice

        if (reversed_mask[current]) {
          // Found reversed path, stop traversing
          // Record the lowest node in the tree that is on the reversed path
          if (current < reversed_path_end)
            reversed_path_end = current;

          break;  //< Stop traversal at reversed path
        }

        current = joints[current].parent; //< Move to next parent
      }
    }
    assert(subtree_mask[reversed_path_end]);

    // Fill in the rest of the reversed path up until reversed_path_end
    current = root_joint_id;
    while (current < reversed_path_end) {
      subtree_mask[current] = true;
      current = joints[current].parent;
    }

    // Traverse down reverse path to collect subtree sizes (reversed parents are children in the child joint's data).
    

    // Calculate subtree sizes per analysis joint using DP
    // Used to know when we need to do DP
    constexpr auto invalid_subtree_size = std::numeric_limits<short unsigned int>::max();
    std::vector<short unsigned int> joint_subtree_sizes(joints.size(), 0);

    // Set base cases from frames
    std::vector<short unsigned int> frame_subtree_sizes(frames.size(), 0);  //< Silly, but the same frame might appear twice
    for (const auto & frame_id : definition_frame_ids) {
      const auto & frame = frames[frame_id];
      const auto joint_id = frame.parent;

      ++frame_subtree_sizes[joint_id];


      auto current_jid = joint_id;
      while (joint_subtree_sizes[current_jid] == 0) {


      }

    }




    // Set base case


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
  std::vector<size_t> tree_pose_indices_;
  /// Final offset to apply in the output pose at index i.
  /// All the output poses we typically care about will have an offset from the closest parent joint that actuates.
  /// Probably std::move()-d from FrameDefinitions
  Isometry3dVector offsets_;

  /// Used to do mapping for non-fixed joints
  ComputeJointTree tree_;

  /// The number of non-constants
  size_t varyings_;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_EIGENFKMAPPER_HPP