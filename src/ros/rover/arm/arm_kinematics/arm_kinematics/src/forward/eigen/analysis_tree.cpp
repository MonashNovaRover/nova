//
// Created by Bailey Chessum on 19/11/2025.
//

#include <arm_kinematics/forward/eigen/analysis_tree.hpp>

namespace arm_kinematics {

AnalysisTree::AnalysisTree(
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
  size_t subtree_joint_count = 0;   //< Number of elements in the subtree, used to construct topological Order<true>
  size_t forward_joint_count = 0;   //< subtree_joint_count - reversed_path_length

  // indices of frames for each definition
  frames_.reserve(definitions.size());
  auto frame_new_to_old = Order(definitions.size(), other.frames_.size());  //< TODO: make better name
  // frame_parents.reserve(definitions.size());
  // frame_origins.reserve(definitions.size());
  for (size_t i = 0; i < definitions.size(); ++i) {
    if (!other.frames_.contains(definitions.parent_link_names[i]))
      throw std::invalid_argument(std::string("Given link name for definition \"") + std::to_string(i) + definitions.parent_link_names[i] + std::string("\" is not in the URDF AnalysisTree."));

    frame_new_to_old[i] = other.frames_[definitions.parent_link_names[i]];
  }

  // Find which elements are in the subtree, and the end of the reversed path as the lowest id in the subtree
  // complexity O(definitions.size() + subtree size)
  auto reversed_path_end_old = root_joint_id;
  for (const auto & frame_id : frame_new_to_old) {
    current = other.frames_[frame_id].parent;  //< current joint id

    while (!subtree_mask[current]) {
      subtree_mask[current] = true; //< Don't traverse the same place twice
      if (current)
        subtree_joint_count++;

      if (reversed_mask_old[current]) {
        // Found reversed path, stop traversing
        // Record the lowest node in the tree that is on the reversed path
        if (current < reversed_path_end_old)
          reversed_path_end_old = current;

        break;  //< Stop traversal at reversed path
      }

      forward_joint_count++;  //< Impossible for the dummy root to be forward
      current = other.joints_[current].parent; //< Move to next parent
    }
  }
  assert(subtree_mask[reversed_path_end_old]);

  // Fill subtree_mask in for the rest of the reversed path up until reversed_path_end_
  current = root_joint_id;
  while (current < reversed_path_end_old) {
    if (!subtree_mask[current]) {
      subtree_mask[current] = true;
      ++subtree_joint_count;
    }
    current = joints_[current].parent;
  }

  RCLCPP_INFO(rclcpp::get_logger("fresh_new_subtree_before_adding_joints"), "reverse path end is %lu", reversed_path_end_old);

  // Create topological order of new joint ids to old joint ids
  assert(subtree_joint_count > 0);
  auto order = Order(subtree_joint_count + 1, other.joints_.size());

  // Create joints
  joints_.reserve(subtree_joint_count + 1); //< +1 for the dummy root

  // Add dummy root joint
  order[0] = 0;
  log(rclcpp::get_logger("fresh_new_subtree_before_adding_joints"));
  joints_.add("", {0});

  // Add reversed root joint off the dummy root
  if (root_joint_id != 0) {
    order.inverse[root_joint_id] = add_joint(
      other.joints_.names[root_joint_id],
      0,
      other.joints_[root_joint_id].joint.reversed(),
      other.frames_[*root_frame_id].origin.inverse()
    );
  }

  // First push back reverse path
  current = root_joint_id;
  while (current > reversed_path_end_old)
  {
    // We have to push the parent rather than the value we just checked in the while statement, so that we can push a
    // reversed_path_end_ of 0 (which has itself as its parent) without accepting an index of 0 in the condition.
    auto inverse_previous_origin = other.joints_[current].origin.inverse();
    current = other.joints_[current].parent;

    if (current == 0)
      break;  //< Changed my mind -- we don't want the dummy root in this order

    // TODO: This will push the dummy root. We probably dont want that.
    const auto & other_joint = other.joints_[current];

    order.inverse[current] = add_joint(
      other.joints_.names[current],
      joints_.size() - 1,
      other_joint.joint.reversed(),
      inverse_previous_origin
    );
  }

  RCLCPP_INFO(rclcpp::get_logger("fresh_new_subtree_started_adding_joints"), "reverse path is %s", order.to_string().c_str());

  // Add all other subtree joints to the ordering, inherit existing topological order from other.joints.
  for (size_t joint_id_old = 1; joint_id_old < other.joints_.size(); ++joint_id_old) //< start at 1 to skip dummy root
  {
    if (!subtree_mask[joint_id_old] || reversed_mask_old[joint_id_old])
      continue;

    const auto & other_joint = other.joints_[joint_id_old];

    // Normal case
    if (!reversed_mask_old[other_joint.parent]) {
      order.inverse[joint_id_old] = add_joint(
        other.joints_.names[joint_id_old],
        order.inverse[other_joint.parent],
        other_joint.joint,
        other_joint.origin
      );
      continue;
    }

    // Case 2 -- Edge case, where we must fold the dummy root into the end of the reversed path
    if (other_joint.parent == 0)  //< parented to the other tree's dummy root
    {
      const auto & reversed_path_end_old_joint = other.joints_[reversed_path_end_old];
      auto origin = reversed_path_end_old_joint.origin.inverse() * other_joint.origin;

      order.inverse[joint_id_old] = add_joint(
        other.joints_.names[joint_id_old],
        order.inverse[reversed_path_end_old], //< TODO: Should we precompute and reuse this value?
        other_joint.joint,
        origin
      );
      continue;
    }

    // Case 1 - The parent of the other_joint's parent in the reversed path (i.e. the current joints to-be parent)
    const auto & reverse_path_grandparent_id = order.inverse[other_joint.parent] - 1;
    // const auto & reverse_path_grandparent_id_old = topological[reverse_path_grandparent_id];
    // const auto & reverse_path_grandparent = other.joints_[reverse_path_grandparent_id];

    // We have to use the origin transform from the new subtree to handle the reversed path root joint (Case 0)
    const auto & other_parent_new = joints_[order.inverse[other_joint.parent]]; //< bad name
    const auto origin = other_parent_new.origin * other_joint.origin;

    order.inverse[joint_id_old] = add_joint(
      other.joints_.names[joint_id_old],
      reverse_path_grandparent_id,
      other_joint.joint,
      origin
    );
  }

  // All joints should now be finished -> move onto constructing frames
  assert(joints_.size() == subtree_joint_count + 1);

  // Calculate all frames to match the given definitions
  for (size_t frame_id = 0; frame_id < definitions.size(); ++frame_id)
  {
    auto frame_id_old = other.frames_[definitions.parent_link_names[frame_id]];
    const auto & frame_old = other.frames_[frame_id_old];
    auto joint_id_old = frame_old.parent;

    const auto & definition_origin = frame_old.origin * definitions.origins[frame_id];

    assert(subtree_mask[joint_id_old]);

    const auto & other_joint = other.joints_[joint_id_old];

    // Normal case
    if (!reversed_mask_old[other_joint.parent]) {
      add_frame(
        "",
        order.inverse[other_joint.parent],
        definition_origin
      );
      continue;
    }

    // Case 2 -- Edge case, where we must fold the dummy root into the end of the reversed path
    if (frame_id_old == 0)  //< parented to the other tree's dummy root
    {
      const auto & reversed_path_end_old_joint = other.joints_[reversed_path_end_old];
      auto origin = reversed_path_end_old_joint.origin.inverse() * definition_origin;

      add_frame(
        "",
        order.inverse[reversed_path_end_old], //< TODO: Should we precompute and reuse this value?
        origin
      );
      continue;
    }

    // Case 1 - The parent of the other_joint's parent in the reversed path (i.e. the current joints to-be parent)
    const auto & reverse_path_grandparent_id = order.inverse[other_joint.parent] - 1;

    // We have to use the origin transform from the new subtree to handle the reversed path root joint (Case 0)
    const auto & other_parent_new = joints_[order.inverse[other_joint.parent]]; //< bad name
    const auto origin = other_parent_new.origin * definition_origin;

    order.inverse[joint_id_old] = add_joint(
      other.joints_.names[joint_id_old],
      reverse_path_grandparent_id,
      other_joint.joint,
      origin
    );
  }

  assert(frames_.size() == definitions.size());
}

void AnalysisTree::sort_joint_children_strategy_a() {
  const std::vector<size_t> subtree_sizes = get_subtree_joint_count();
  std::vector<size_t> leaf_child_count(joints_.size(), 0);

  size_t i = joints_.size();
  for (auto it = joints_.data.rbegin(); it != joints_.data.rend(); ++it) {
    --i;
    auto & joint = *it;

    if (joint.children.empty())
      continue;

    // Move all leaves to the beginning of children
    auto child_leaf_it = joint.children.begin();  //< Making this an iterator lets us reuse it as begin() in std::sort

    for (size_t j = 0; j < joint.children.size(); ++j) {
      // const auto & child = joints_[j];

      // Move leaf nodes to the start with the rest of the leaf children
      if (subtree_sizes[j] == 1) {
        std::swap(*child_leaf_it, joint.children[j]);
        ++child_leaf_it;
        ++leaf_child_count[i];
      }
    }

    std::sort(child_leaf_it, joint.children.end(), [&subtree_sizes](auto a, auto b) {
      return subtree_sizes[a] < subtree_sizes[b];
    });
  }
}

Order<> AnalysisTree::sort_joints(bool sort_children) {
  sorted_frames_ = false;

  if (sort_children)
    sort_joint_children_strategy_a();

  log(rclcpp::get_logger("sort_joints"));

  Order order(joints_.size(), joints_.size());

  order[0] = 0;
  size_t order_count = 1;   //< Current index in order to set
  // size_t current = 0;

  for (const auto & joint_id : order) {
    const auto & joint = joints_[joint_id];

    for (const auto child_id : joint.children) {
      assert(child_id < joints_.size());
      order[order_count] = child_id;
      ++order_count;
    }
  }

  RCLCPP_INFO(rclcpp::get_logger("anal_tree"), "forward %s", order.to_string().c_str());
  RCLCPP_INFO(rclcpp::get_logger("anal_tree"), "inverse %s", order.inverse.to_string().c_str());

  sort_joints(order);
  sorted_joints_ = true;
  return order;
}

}

