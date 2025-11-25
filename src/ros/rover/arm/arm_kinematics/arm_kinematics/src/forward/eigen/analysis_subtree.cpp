//
// Created by nova on 22/11/25.
//

#include <arm_kinematics/forward/eigen/analysis_subtree.hpp>

namespace arm_kinematics
{

AnalysisSubtree::AnalysisSubtree(
  const AnalysisTree& tree,
  const std::string& root_name,
  const FrameDefinitions& definitions)
: AnalysisSubtree(tree)
{
  const auto & joints = tree.get_joints();
  const auto & frames = tree.get_frames();

  // First figure out root
  const auto root_frame_id = frames.get(root_name);
  if (!root_frame_id.has_value())
    throw std::invalid_argument("Given root_name is not in the AnalysisTree");
  root_joint_id_ = frames[*root_frame_id].parent;

  // Find reversed path (path from root_joint_id_ to 0, 0 is the URDF root)
  reversed_mask_old_ = std::vector(joints.size(), false);
  size_t reversed_joint_count = 0; //< Number of elements in the subtree, used to construct topological Order<true>
  size_t current = root_joint_id_;
  while (current != 0) {
    reversed_mask_old_[current] = true;
    current = joints[current].parent;
  }
  reversed_mask_old_[0] = true;

  // First find out what is in the subtree
  std::vector<bool> subtree_mask(joints.size(), false);
  size_t subtree_joint_count = 0; //< Number of elements in the subtree, used to construct topological Order<true>
  size_t forward_joint_count = 0; //< subtree_joint_count - reversed_path_length

  // indices of frames for each definition
  std::vector<size_t> definition_frame_ids(definitions.size());
  frame_parents.reserve(definitions.size());
  frame_origins.reserve(definitions.size());
  for (size_t i = 0; i < frames.size(); ++i) {
    definition_frame_ids[i] = frames[definitions.parent_link_names[i]];
  }

  // Find which elements are in the subtree, and the end of the reversed path as the lowest id in the subtree
  // complexity O(definitions.size() + subtree size)
  reversed_path_end_ = root_joint_id_;
  for (const auto & frame_id : definition_frame_ids) {
    current = frames[frame_id].parent;  //< current joint id

    while (!subtree_mask[current]) {
      subtree_mask[current] = true; //< Don't traverse the same place twice
      subtree_joint_count++;

      if (reversed_mask_old_[current]) {
        // Found reversed path, stop traversing
        // Record the lowest node in the tree that is on the reversed path
        if (current < reversed_path_end_)
          reversed_path_end_ = current;

        break;  //< Stop traversal at reversed path
      }

      forward_joint_count++;
      current = joints[current].parent; //< Move to next parent
    }
  }
  assert(subtree_mask[reversed_path_end_]);

  // Fill subtree_mask in for the rest of the reversed path up until reversed_path_end_
  current = root_joint_id_;
  while (current < reversed_path_end_) {
    if (!subtree_mask[current]) {
      subtree_mask[current] = true;
      ++subtree_joint_count;
    }
    current = joints[current].parent;
  }

  // Create topological order
  assert(subtree_joint_count > 0);
  topological = Order(subtree_joint_count);
  reversed_path_old_ = Order(subtree_joint_count - forward_joint_count);
  assert(reversed_path_old_.size() >= 1); //< Must contain at least the root node

  // First push back reverse path
  reversed_path_old_[0] = root_joint_id_;
  topological[0] = root_joint_id_;
  size_t topological_length = 1;      //< Number of elements we have pushed back onto topological

  current = root_joint_id_;
  while (current > reversed_path_end_)
  {
    // We have to push the parent rather than the value we just checked in the while statement, so that we can push a
    // reversed_path_end_ of 0 (which has itself as its parent) without accepting an index of 0 in the condition.
    current = joints[current].parent;
    reversed_path_old_[topological_length] = current;
    topological[topological_length] = current;  //< Push back reversed path element
    ++topological_length;
  }
  size_t reversed_path_length = topological_length;

  // Add all other subtree joints to the ordering, inherit existing topological order from joints.
  for (size_t i = 0; i < joints.size(); ++i)
  {
    if (!subtree_mask[i] || reversed_mask_old_[i])
      continue;

    topological[topological_length++] = i;  //< Push back forward element
  }

  // topological order now complete
  assert(topological_length == topological.size());
  assert(topological_length == subtree_joint_count);

  subtree_sizes_old_ = std::vector<size_t>(joints.size());
  const auto & inverse_reversed_path_old = get_inverse_reversed_path_old();

  for (size_t i = 0; i < frames.size(); i++) {
    const auto & frame_id = definition_frame_ids[i];
    const auto & joint_id = frames[frame_id].parent;

    if (!reversed_mask_old_[joint_id]) {
      // Forward joint -- easy case
      frame_parents[i] = joint_id;
      frame_origins[i] = definitions.origins[i];
      subtree_sizes_old_[joint_id] += 1;
      continue;
    }

    if (joint_id == root_joint_id_) {
      // This has no previous reversed joint id, as it is at the start of the path.
      // It instead needs to become root relative, so we give it an invalid value
      frame_parents[i] = std::numeric_limits<size_t>::max();
      frame_origins[i] = frames[*root_frame_id].origin.inverse() * frames[frame_id].origin * definitions.origins[i];
      // TODO: This needs to be put at the end of the definition order because it is fixed
      break;
    }

    // attach to previous joint in the parent path
    const auto & previous_reversed_joint_id =

  }
  // subtree_sizes_old_ now contains the number of immediate child frames from FrameDefinitions

  for (auto it = topological.rbegin(); it != topological.rend(); ++it)
  {
    const auto & joint_id = *it;
    const auto & joint = joints[joint_id];

    if (reversed_mask_old_[joint_id]) {

      continue;
    }



  }




}

} // arm_kinematics