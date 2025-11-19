//
// Created by Bailey Chessum on 19/11/2025.
//

#ifndef ARM_KINEMATICS_TREEORDERING_HPP
#define ARM_KINEMATICS_TREEORDERING_HPP

#include <vector>
#include <set>
#include <string>
#include <map>
#include <optional>
#include <rclcpp/logging.hpp>
#include <algorithm>
#include <urdf/model.h>

#include "eigen_fk_tree.hpp"
#include "name_to_vector.hpp"
#include <arm_kinematics/frame_definitions.hpp>

namespace arm_kinematics::detail {

struct EigenFKMapperProps {
  std::vector<EigenFKTree::JointType> joint_types;
  std::vector<std::string> joint_names;

  Vector3dVector joint_axes;
  Isometry3dVector origins;
  std::vector<size_t> parents;
  size_t root_relative_count;

  Isometry3dVector mapper_offsets;
  std::vector<size_t> tree_pose_indices;
};

/**
 * Gets the equivalent EigenFKTree type for a urdf joint
 * \param joint The joint
 * \return std::nullopt, or a joint type
 */
inline std::optional<EigenFKTree::JointType> get_type(const urdf::JointConstSharedPtr & joint) {
  switch (joint->type) {
    case urdf::Joint::REVOLUTE:
      return EigenFKTree::JointType::REVOLUTE;
    case urdf::Joint::PRISMATIC:
      return EigenFKTree::JointType::PRISMATIC;
    case urdf::Joint::CONTINUOUS:
      return EigenFKTree::JointType::CONTINUOUS;
    default:
      return std::nullopt;
  }
}

inline Eigen::Vector3d to_eigen(const urdf::Vector3 & p)
{
  return {p.x, p.y, p.z};
}

inline Eigen::Isometry3d to_eigen(const urdf::Pose & p)
{
  Eigen::Isometry3d T = Eigen::Isometry3d::Identity();
  T.translation() = Eigen::Vector3d(p.position.x, p.position.y, p.position.z);

  double x, y, z, w;
  p.rotation.getQuaternion(x, y, z, w);
  Eigen::Quaterniond q(w, x, y, z);
  T.linear() = q.toRotationMatrix();
  return T;
}

class TreeOrdering {
public:
  /// Traverses up the fake root link's parents, adding them in reverse, where the parent-child relationships are
  /// swapped making the fake root the actual root of the tree we are constructing
  explicit TreeOrdering(const urdf::LinkConstSharedPtr & fake_root, size_t capacity) {
    if (!fake_root)
      return;

    // Add the fake root link
    links_.add(fake_root->name, {0});
    root_path_length_ = 1;

    // Construct an equivalent backwards chain to the actual root
    auto previous = fake_root;
    size_t previous_id = 0;   //< The fake root will always be at index 0
    auto previous_origin = Eigen::Isometry3d::Identity();

    auto current = fake_root->getParent();

    while (current && previous->parent_joint) {
      // Construct joint in reverse, using the previous joint's origin transform, as joints get applied after origin
      const auto joint_id = try_create_reversed_joint(previous->parent_joint);
      auto current_id = links_.add(current->name, {
        previous_id,
        joint_id,
        previous_origin.inverse()   //< Reversed as we have reversed the joint. Maps from URDF child->parent frame, so
      });                           //  the joint axis should still be in the correct reference frame.
      ++root_path_length_;

      // Register child
      links_[previous_id].children.emplace(current_id);

      // Traverse to next parent
      previous = current;
      previous_id = current_id;
      previous_origin = to_eigen(previous->parent_joint->parent_to_joint_origin_transform);

      current = current->getParent();
    }
  }

  TreeOrdering(
    const urdf::Model & model,
    const std::string & fake_root_name,
    FrameDefinitions frames)
  : TreeOrdering(model.getLink(fake_root_name), model.links_.size())
  {
    if (empty())
      return; //< Fake root link not found in URDF

    // Move members from frames
    frame_origins_ = std::move(frames.origins);
    frame_links_.resize(frame_origins_.size());
    const std::vector frame_names_{std::move(frames.parent_link_names)};

    // Get link ids for each frame
    for (size_t i = 0; i < frame_links_.size(); ++i) {
      const auto link_id = add(model.getLink(frame_names_[i]));
      // frame_links[i] = link_id;  //< This step is redundant, as it will later be set from backlinks

      // Add backlink from link to frame
      links_[link_id].frames.insert(i);
    }
  }

  /**
   * Post condition: No leaf links will be missing a joint
   * Once you run this you can't add anythign else to the ordering
   */
  EigenFKMapperProps finish() {
    // deletion_mask[i] is true if the link should be deleted
    std::vector<bool> link_deletion_mask(links_.size());
    assert(link_deletion_mask.size() == links_.size());  //< Just double-checking my understanding
    std::vector<bool> joint_deletion_mask(joints_.size());

    uint links_to_delete = 0;
    uint joints_to_delete = 0;

    // Squash all fixed links
    for (size_t link_id = 0; link_id < links_.size(); ++link_id) {
      links_to_delete += link_deletion_mask[link_id] = squash(link_id);
    }

    // Find the end of the root path that is actually used
    for (size_t i = root_path_length_ - 1; i > 0; --i) {
      if (link_deletion_mask[i]) //< Anything marked for deletion will fail the is_necessary test
        continue;

      const auto & link = links_[i];

      // The number of children this link would've had if no additional children were added to it
      const size_t base_children = i == root_path_length_ - 1 ? 0 : 1;

      // if is necessary
      if (link.children.size() > base_children || !link.frames.empty()) {
        break;
      }

      // mark anything unnecessary for deletion
      link_deletion_mask[i] = true;
      if (link.joint.has_value()) {
        joint_deletion_mask[link.joint.value()] = true;
        ++joints_to_delete;
      }
    }

    std::vector<size_t> new_to_old_links(links_.size() - links_to_delete);
    size_t new_link_id = 0;
    for (size_t old_link_id = 0; old_link_id < links_.size(); ++old_link_id) {
      if (link_deletion_mask[old_link_id])
        continue;

      new_to_old_links[new_link_id] = old_link_id;
      ++new_link_id;
    }

    std::vector<size_t> new_to_old_joints(joints_.size() - joints_to_delete);
    size_t new_joint_id = 0;
    for (size_t old_joint_id = 0; old_joint_id < joints_.size(); ++old_joint_id) {
      if (joint_deletion_mask[old_joint_id])
        continue;

      new_to_old_joints[new_joint_id] = old_joint_id;
      ++new_joint_id;
    }

    ///




    // Build the props!

    std::vector<EigenFKTree::JointType> joint_types(joints_.size() - joints_to_delete);
    for (size_t i = 0; i < joint_types.size(); ++i)
      joint_types[i] = joints_[new_to_old_joints[i]].type;

    std::vector<std::string> joint_names(joints_.size() - joints_to_delete);
    for (size_t i = 0; i < joint_names.size(); ++i)
      joint_names[i] = joints_.names[new_to_old_links[i]];

    Vector3dVector joint_axes(joints_.size() - joints_to_delete);
    for (size_t i = 0; i < joint_axes.size(); ++i)
      joint_axes[i] = joints_[new_to_old_joints[i]].axis;

    Isometry3dVector origins;
    std::vector<size_t> parents;
    size_t root_relative_count;

    Isometry3dVector mapper_offsets;
    std::vector<size_t> tree_pose_indices;
  }

  /**
   * Add a link and all of its parents to the tree
   * \param link controller names in the order they need to be activated
   * \returns True if the link was added, false otherwise.
   */
  bool add(const urdf::LinkConstSharedPtr & link)
  {
    if (!link || empty())
      return false;

    if (links_.contains(link->name))
      return true;  //< No action needed

    // Assume the URDF root has already been added -- Missing parent means this is invalid
    if (!link->getParent() || !link->parent_joint)
      return false;

    find_or_create_link(link);
    return true;
  }

  /**
   * Ensures the controllers are sorted
   */
  void sort() {
    // if (!is_sorted_)
      order();
  }

  /// If true, the fake root link was invalid
  [[nodiscard]] constexpr bool empty() const {
    return links_.size() == 0;
  }

  /// Get the link id for each frame
  [[nodiscard]] std::vector<size_t> get_frame_link_ids() {
    std::vector<size_t> frame_links(frame_origins_.size());

    // Use backlinks to update frames to point to the correct link
    for (size_t link_id = 0; link_id < links_.size(); ++link_id) {
      for (const auto & frame_id : links_[link_id].frames) {
        frame_links_[frame_id] = link_id;
      }
    }

    return frame_links;
  }

  [[nodiscard]] constexpr const Isometry3dVector & get_frame_origins() const {
    return frame_origins_;
  }

  [[nodiscard]] Isometry3dVector get_tree_origins() {
    Isometry3dVector tree_origins(links_.size());

    for (size_t i = 0; i < links_.size(); ++i)
      tree_origins[i] = links_[i].origin;

    return tree_origins;
  }

private:
  struct JointDescription {
    EigenFKTree::JointType type;
    Eigen::Vector3d axis;
  };

  struct LinkHandle
  {
    size_t parent = 0; //< 0 is the root

    /// The type represented by this joint. Fixed if std::nullopt
    std::optional<size_t> joint = std::nullopt;
    /// Defines the reference frame of the link relative to the parent
    Eigen::Isometry3d origin = Eigen::Isometry3d::Identity();

    std::set<size_t> children{};

    /// The ids of all frames that point to this handle
    std::set<size_t> frames{};

    bool remove_child(const size_t link_id) {
      const auto it = children.find(link_id);
      if (it == children.end())
        return false; //< This has already been removed!

      children.erase(it);
      return true;
    }
  };

  /**
   * Gets the id of a link handle in the
   * \param link
   * \return
   * \warning link must be valid
   */
  size_t find_or_create_link(const urdf::LinkConstSharedPtr & link)
  {
    if (!link)
      return 0;

    if (links_.contains(link->name))
      return links_[link->name];

    const size_t idx = create_link_aux(link);
    links_[idx].parent = ensure_parent_recursive(link->getParent(), idx);

    return idx;
  }

  /// Creates a link WITH assigning parent, end ensures child is a child of the given link
  size_t ensure_parent_recursive(const urdf::LinkConstSharedPtr & link, size_t child) {
    assert(link);
    if (!link)
      return 0;

    if (links_.contains(link->name)) {
      const auto idx = links_[link->name];
      links_[idx].children.emplace(child);   //< ensure child is a child of parent
      return idx;
    }

    // Create the parent
    const auto idx = create_link_aux(link);
    links_[idx].children.emplace(child);
    links_[idx].parent = ensure_parent_recursive(link->getParent(), idx);
    return idx;
  }

  /// Creates a link without assigning parent
  size_t create_link_aux(const urdf::LinkConstSharedPtr & link) {
    assert(link);

    const auto origin = link->parent_joint
      ? to_eigen(link->parent_joint->parent_to_joint_origin_transform)
      : Eigen::Isometry3d::Identity();

    const auto new_id = links_.size();
    links_.add(link->name, LinkHandle{
      new_id,
      find_or_create_joint(link->parent_joint),
      origin
    });
    return new_id;
  }

  /// Simplification step that removes a fixed joint from the tree.
  /// Only modifies links with a joint == std::nullopt
  /// \returns true if the link was squashed
  bool squash(const size_t link_id) {
    if (link_id == 0)
      return false; //< We cannot squash the root

    auto & link = links_[link_id];
    if (link.joint.has_value())
      return false;

    auto & parent = links_[links_[link_id].parent];

    // Remove self from parent
    parent.remove_child(link_id);

    // Migrate children to parent
    for (auto & child_id : link.children) {
      auto & child = links_[child_id];
      child.parent = link.parent;
      child.origin = link.origin * child.origin;
    }
    parent.children.merge(link.children);
    link.children = std::set<size_t>{}; //< Clear children

    // Migrate frames to parent
    for (auto & frame_id : link.frames) {
      auto & frame_origin = frame_origins_[frame_id];
      frame_origin = link.origin * frame_origin;

      frame_links_[frame_id] = link.parent;
    }
    parent.frames.merge(link.frames);
    link.frames = std::set<size_t>{};   //< Clear frames

    return true;
  }

  /// Swaps the links at index a and index b
  /// Note:
  void swap(const size_t a, const size_t b) {
    if (a == b)
      return;

    const auto & link_a = links_[a];
    const auto & link_b = links_[b];

    // Swap parents
    auto & parent_a = links_[link_a.parent];
    auto & parent_b = links_[link_b.parent];
    // Remove from old parents
    parent_a.remove_child(a);
    parent_b.remove_child(b);
    // Add to new parents
    parent_b.children.emplace(a);
    parent_a.children.emplace(b);

    // Fix children
    for (const auto child : link_a.children)
      links_[child].parent = b;
    for (const auto child : link_b.children)
      links_[child].parent = a;

    // swap the actual data
    links_.swap(a, b);
  }

  std::optional<size_t> find_or_create_joint(const urdf::JointConstSharedPtr & joint)
  {
    if (!joint)
      return std::nullopt;

    const auto joint_type = get_type(joint);
    if (!joint_type.has_value())
      return std::nullopt; //< This guard can be put before or after the lookup, but thought this was cheaper

    // Try find existing joint
    if (joints_.contains(joint->name))
      return joints_[joint->name];

    // Otherwise create the joint
    return joints_.add(joint->name, JointDescription{
      joint_type.value(),
      to_eigen(joint->axis)
    });
  }

  std::optional<size_t> try_create_reversed_joint(const urdf::JointConstSharedPtr & joint) {
    if (!joint)
      return std::nullopt;

    const auto joint_type = get_type(joint);
    if (!joint_type.has_value())
      return std::nullopt; //< This guard can be put before or after the lookup, but thought this was cheaper

    // Otherwise create the joint, reversing the axis
    return joints_.add(joint->name, JointDescription{
      joint_type.value(),
      -to_eigen(joint->axis)  //< This is already in the child joint space, so should be fine?
    });
  }

  NameToVector<JointDescription> joints_{};
  NameToVector<LinkHandle> links_{};

  std::vector<size_t> frame_links_{}; //< TODO: Remove, as we don't want to maintain its invariants
  Isometry3dVector frame_origins_{};

  /// Number of elements in the reversed chain from the fake root to the URDF root.
  /// When 0, it is not safe to add new links, as reversed fixed frames have now been folded
  size_t root_path_length_ = 0;
};

}

#endif //ARM_KINEMATICS_TREEORDERING_HPP
