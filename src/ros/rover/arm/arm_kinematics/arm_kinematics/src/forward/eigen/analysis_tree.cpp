//
// Created by Bailey Chessum on 19/11/2025.
//

#include <arm_kinematics/forward/eigen/analysis_tree.hpp>

namespace arm_kinematics::detail {
void AnalysisTree::order() {
  std::vector<size_t> in_degrees{};   //< stores for each handle the number of dependencies of the handle
  in_degrees.reserve(handles_.size());

  std::vector<size_t> ordering{};
  ordering.reserve(handles_.size());

  size_t root_count = 0;

  // Get the in_degree for every handle, and keep track of any root handles
  for (size_t i = 0; i < handles_.size(); ++i)
  {
    in_degrees.emplace_back(handles_[i].dependencies->size());
    if (in_degrees[i] == 0)
    {
      // This element is a root, so we can safely add it to the ordering_
      ordering.emplace_back(i);
      ++root_count;
    }
  }

  // Iterate over every element in the ordering_ to try add its children to the ordering_
  for (size_t i = 0; i < ordering.size(); ++i)
  {
    const auto id = ordering[i];
    const auto& children = handles_[id].children;

    for (const auto& child : *children)
    {
      // decrement the in_degree
      const auto child_in_degree = --in_degrees[child];

      // If we decremented to 0, we can safely add it to the ordering_
      if (child_in_degree == 0)
        ordering.emplace_back(child);
    }
  }

  if (ordering.size() != handles_.size())
  {
    // Uh oh! The graph is not acyclic!
    const auto logger = rclcpp::get_logger("controller_ordering");
    RCLCPP_ERROR(logger,
                 "Failed to create a well-ordering_ for controller activation! You had a cycle in activation order "
                 "that might involve these controller names:");

    // Log the names of all handles_ with an in_degree > 0
    // And incorporate stray handles anyway
    for (size_t j = 0; j < in_degrees.size(); ++j)
    {
      if (in_degrees[j] == 0)
        continue;
      RCLCPP_ERROR(logger, "  - \"%s\"", handles_[j].name.c_str());

      // Incorporate the stray handle
      ordering.emplace_back(j);
    }

    RCLCPP_ERROR(logger,
                 "All controllers need to be specified in the same order for each control mode. You have likely "
                 "changed the order you've listed controllers between different control modes.");
  }

  // Sort handles to match order
  std::vector<size_t> inverse_ordering{};   //< Maps old ids to ordered ids
  inverse_ordering.resize(ordering.size(), 0);

  for (size_t i = 0; i < ordering.size(); ++i) {
    inverse_ordering[ordering[i]] = i;
  }

  // Move handles to match the new ordering_
  std::vector<Handle> new_handles{};
  for (size_t i = 0; i < ordering.size(); ++i) {
    const auto& old_handle = handles_[ordering[i]];

    auto dependencies = std::make_unique<std::set<size_t>>();
    for (const auto dependency : *old_handle.dependencies)
      dependencies->insert(ordering[dependency]);

    auto children = std::make_unique<std::set<size_t>>();
    for (const auto child : *old_handle.children)
      children->insert(ordering[child]);

    new_handles.emplace_back(Handle{old_handle.name, std::move(dependencies), std::move(children)});
  }
  handles_ = std::move(new_handles);

  // Update the ids in the name_to_id_ map to match new ordering_
  for (auto& [_, id] : name_to_id_) {
    id = inverse_ordering[id];
  }

  is_sorted_ = true;
}
}
