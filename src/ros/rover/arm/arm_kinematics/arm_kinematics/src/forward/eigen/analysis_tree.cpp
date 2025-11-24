//
// Created by Bailey Chessum on 19/11/2025.
//

#include <arm_kinematics/forward/eigen/analysis_tree.hpp>

namespace arm_kinematics {

arm_kinematics::AnalysisTree::AnalysisTree(
  const AnalysisTree& other,
  const std::string& root_name,
  const FrameDefinitions& definitions)
{
  // First figure out root
  const auto root_frame_id = other.frames_.get(root_name);
  if (!root_frame_id.has_value())
    throw std::invalid_argument("Given root_name is not in the AnalysisTree");
  auto root_joint_id = other.frames_[*root_frame_id].parent;

  // Find reversed path (path from root_joint_id_ to 0, 0 is the URDF root)
  auto reversed_mask_old = std::vector(other.joints_.size(), false);
  size_t reversed_joint_count = 0; //< Number of elements in the subtree, used to construct topological Order<true>
  size_t current = root_joint_id;
  while (current != 0) {
    reversed_mask_old[current] = true;
    current = other.joints_[current].parent;
  }
  reversed_mask_old[0] = true;

  // First find out what is in the subtree
  std::vector<bool> subtree_mask(other.joints_.size(), false);
  size_t subtree_joint_count = 0; //< Number of elements in the subtree, used to construct topological Order<true>
  size_t forward_joint_count = 0; //< subtree_joint_count - reversed_path_length

  // indices of frames for each definition
  frames_.reserve(definitions.size());
  auto frame_new_to_old = Order<size_t>(definitions.size(), other.frames_.size());
  // frame_parents.reserve(definitions.size());
  // frame_origins.reserve(definitions.size());
  for (size_t i = 0; i < other.frames_.size(); ++i) {
    frame_new_to_old[i] = other.frames_[definitions.parent_link_names[i]];
  }

  // Find which elements are in the subtree, and the end of the reversed path as the lowest id in the subtree
  // complexity O(definitions.size() + subtree size)
  auto reversed_path_end = root_joint_id;
  for (const auto & frame_id : frame_new_to_old) {
    current = other.frames_[frame_id].parent;  //< current joint id

    while (!subtree_mask[current]) {
      subtree_mask[current] = true; //< Don't traverse the same place twice
      subtree_joint_count++;

      if (reversed_mask_old[current]) {
        // Found reversed path, stop traversing
        // Record the lowest node in the tree that is on the reversed path
        if (current < reversed_path_end)
          reversed_path_end = current;

        break;  //< Stop traversal at reversed path
      }

      forward_joint_count++;
      current = other.joints_[current].parent; //< Move to next parent
    }
  }
  assert(subtree_mask[reversed_path_end]);

  // Fill subtree_mask in for the rest of the reversed path up until reversed_path_end_
  current = root_joint_id;
  while (current < reversed_path_end) {
    if (!subtree_mask[current]) {
      subtree_mask[current] = true;
      ++subtree_joint_count;
    }
    current = joints_[current].parent;
  }

  // Create topological order
  assert(subtree_joint_count > 0);
  auto topological = Order(subtree_joint_count + 1, other.joints_.size());

  // Create joints
  joints_.reserve(subtree_joint_count + 1); //< +1 for the dummy root

  // Add dummy root joint
  topological[0] = 0;
  joints_.add("", {0});
  joints_[0].children.insert(1);  //< Reversed root joint

  // Add reversed root joint
  topological[
    add_joint(
      other.joints_.names[root_joint_id],
      joints_.size() - 1,
      other.joints_[root_joint_id].joint.reversed(),
      other.frames_[*root_frame_id].origin.inverse()
    )
  ] = root_joint_id;

  // First push back reverse path
  // size_t topological_length = 2;      //< Number of elements we have pushed back onto topological

  current = root_joint_id;
  while (current > reversed_path_end)
  {
    // We have to push the parent rather than the value we just checked in the while statement, so that we can push a
    // reversed_path_end_ of 0 (which has itself as its parent) without accepting an index of 0 in the condition.
    auto inverse_previous_origin = other.joints_[current].origin.inverse();
    current = other.joints_[current].parent;
    // topological[topological_length++] = current;  //< Push back reversed path element

    // TODO: This will push the dummy root. We probably dont want that.
    const auto & other_joint = other.joints_[current];

    topological[
      add_joint(
        other.joints_.names[current],
        joints_.size() - 1,
        other_joint.joint.reversed(),
        inverse_previous_origin
      )
    ] = root_joint_id;
  }
  // size_t reversed_path_length = topological_length;

  // Add all other subtree joints to the ordering, inherit existing topological order from other.joints.
  for (size_t i = 0; i < other.joints_.size(); ++i)
  {
    if (!subtree_mask[i] || reversed_mask_old[i])
      continue;

    topological[topological_length + 1] = i;  //< Push back forward element
    ++topological_length;
  }

  // topological order now complete
  assert(topological_length == topological.size());
  assert(topological_length == subtree_joint_count);

  for (size_t i = 0; i < frames_.size(); i++) {
    const auto & old_frame_id = frame_new_to_old.inverse[i];
    const auto & old_joint_id = other.frames_[old_frame_id].parent;

    if (!reversed_mask_old[old_joint_id]) {
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

}

