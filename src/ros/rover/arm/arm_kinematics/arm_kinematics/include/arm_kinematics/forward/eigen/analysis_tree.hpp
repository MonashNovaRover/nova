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
#include <urdf/model.h>
#include "compute_joint_tree.hpp"
#include "name_to_vector.hpp"
#include <arm_kinematics/frame_definitions.hpp>
#include "arm_kinematics/utilities/order.hpp"

namespace arm_kinematics {

/**
 * Gets the equivalent ComputeJointTree type for a URDF joint
 * \param joint The joint
 * \return std::nullopt, or a joint type
 */
inline std::optional<ComputeJointTree::JointType> get_type(const urdf::JointConstSharedPtr & joint) {
  switch (joint->type) {
    case urdf::Joint::REVOLUTE:
      return ComputeJointTree::JointType::REVOLUTE;
    case urdf::Joint::PRISMATIC:
      return ComputeJointTree::JointType::PRISMATIC;
    case urdf::Joint::CONTINUOUS:
      return ComputeJointTree::JointType::CONTINUOUS;
    default:
      return std::nullopt;
  }
}

/**
 * Gets the equivalent ComputeJointTree type for a URDF joint
 * \param joint The joint
 * \return std::nullopt, or a joint type
 */
inline std::optional<ComputeJointTree::JointType> get_type(const urdf::Joint & joint) {
  switch (joint.type) {
  case urdf::Joint::REVOLUTE:
    return ComputeJointTree::JointType::REVOLUTE;
  case urdf::Joint::PRISMATIC:
    return ComputeJointTree::JointType::PRISMATIC;
  case urdf::Joint::CONTINUOUS:
    return ComputeJointTree::JointType::CONTINUOUS;
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

/**
 * A class used to convert a urdf::Model into a representation that can later be used by a \c ComputeFrameTreeBuilder to
 * construct \c ComputeJointTree and \c ComputeFrameTree instances.
 *
 * Joint 0 is always a dud.
 *
 * Invariants:
 *   - Always topologically sorted
 *   - Always forward constructed (No reversed joints/links)
 *     - Reversing to a fake root is the responsibility of the <<Insert Constructor Here>>
 */
class AnalysisTree {
public:
  /**
   * Data to describe a joint
   */
  struct JointDescription {
    ComputeJointTree::JointType type = ComputeJointTree::JointType::CONTINUOUS;
    Eigen::Vector3d axis = Eigen::Vector3d::Zero();

    JointDescription() = default;
    explicit JointDescription(const urdf::Joint & joint)
    : type(get_type(joint).value_or(ComputeJointTree::JointType::CONTINUOUS)), axis(to_eigen(joint.axis)) {}
    explicit JointDescription(const urdf::Joint * joint)
    {
      *this = joint ? JointDescription(*joint) : JointDescription();
    }
  };

  /**
   * Defines a link with a joint in the tree. Fixed links are considered to be 'frames'.
   */
  struct Joint
  {
    /// ID of the frame that this link actuates relative to.
    /// If 0, it is relative to the root. The root will have itself as the parent.
    size_t parent = 0; //< 0 is the root, and is always a dummy
    /// Distance from the root node (id = 0)
    size_t depth = 0;

    /// The type represented by this joint. Fixed if std::nullopt
    JointDescription joint{};
    /// Defines the reference frame of the link relative to the parent
    Eigen::Isometry3d origin = Eigen::Isometry3d::Identity();

    /// The IDs of all other frames that actuate relative to this link. All elements of this set should be larger than
    /// this link's id.
    std::set<size_t> children{};
    /// The ids of all frames that point to this handle
    std::set<size_t> frames{};

    /**
     * Removes a child from children
     * @param link_id The child link's id to remove
     * @returns true if the child was removed, false otherwise
     */
    bool remove_child(const size_t link_id) {
      const auto it = children.find(link_id);
      if (it == children.end())
        return false; //< This has already been removed!

      children.erase(it);
      return true;
    }

    /**
     * Removes a frame from frames
     * @param link_id The frame's id to remove
     * @returns true if the frame was removed, false otherwise
     */
    bool remove_frame(const size_t link_id) {
      const auto it = frames.find(link_id);
      if (it == frames.end())
        return false; //< This has already been removed!

      frames.erase(it);
      return true;
    }
  };

  /**
   * A non-actuated link relative to some actuated link
   */
  struct Frame
  {
    /// The ID of the link this is relative
    size_t parent = 0;
    /// The transform from the parent link to this frame
    Eigen::Isometry3d origin;
  };

  explicit AnalysisTree(const urdf::Model & model)
  {
    // Reserve joints_, assume 1 link for each non-fixed joint
    size_t joint_count = 0;
    for (const auto & [name, joint] : model.joints_)
      if (joint->type != urdf::Joint::FIXED)  //< Our tree model does not consider fixed joints to be joints
        joint_count++;
    joints_.reserve(joint_count + 1);  //< +1 for the dummy root
    // Add the dummy root
    joints_.add("", {});

    // Create frames for each link in model
    frames_.reserve(model.links_.size());
    for (const auto & [name, link] : model.links_)
      frames_.add(name, to_frame(link));
  }

  explicit AnalysisTree(
    const AnalysisTree & tree,
    const std::string & root_name,
    const FrameDefinitions & definitions);

  Frame to_frame(const urdf::LinkConstSharedPtr & link) {
    auto origin = Eigen::Isometry3d::Identity();
    const auto joint = find_next_non_fixed_joint(link.get(), origin);

    return {
      find_or_create_joint_link(joint),
      origin
    };
  }

  // Accessors
  [[nodiscard]] const NameToVector<Joint> & get_joints() const noexcept { return joints_; }
  [[nodiscard]] const NameToVector<Frame> & get_frames() const noexcept { return frames_; }

private:
  /**
   * Finds the closest parent joint that is not fixed, accumulating fixed joint offsets in accumulator.
   * \param current The link to find the closest actuated parent joint of.
   * \param accumulator The isometry to accumulate any fixed joint offsets into. Value is still useful, even when
   * nullopt is returned.
   * \return pointer to the joint's child link or nullptr if there is no joint, and this is relative to the root
   */
  static urdf::Link const * find_next_non_fixed_joint(urdf::Link const * current, Eigen::Isometry3d & accumulator) {
    // I unwrapped an originally recursive function
    while (current && current->parent_joint)
    {
      // Base case -- has non-fixed joint parent
      if (current->parent_joint->type != urdf::Joint::FIXED)
        return current;

      // Recursive case
      accumulator = to_eigen(current->parent_joint->parent_to_joint_origin_transform) * accumulator;
      current = current->getParent().get();
    }

    return nullptr;
  }

  /**
   * Creates a joint link for the given child link's urdf::Joint, or returns the existing construction from joints_.
   * \param child_link The link that is the immediate child of the non-fixed urdf::Joint being converted
   * \return 0 if child_link or its parent joint are nullptr, or the id of the link in links_.
   */
  size_t find_or_create_joint_link(urdf::Link const * child_link)
  {
    if (!child_link || !child_link->parent_joint)
      return 0; //< dummy root

    assert(child_link->parent_joint->type != urdf::Joint::FIXED);

    // Check for existing construction.
    if (joints_.contains(child_link->parent_joint->name))
      return joints_[child_link->parent_joint->name];

    // To keep in topological order, we must ensure the parent exists first
    auto origin = Eigen::Isometry3d::Identity();
    const auto grandparent_joint = find_next_non_fixed_joint(child_link, origin); //< may be nullptr!
    const size_t parent_id = find_or_create_joint_link(grandparent_joint);                     //< handles nullptr

    // Create new link
    const size_t id = joints_.add(child_link->parent_joint->name, {
      parent_id,
      joints_[parent_id].depth + 1,
      JointDescription(*child_link->parent_joint),
      origin,
    });

    // Register as child of parent
    assert(id > parent_id);
    // We know id will be last, so we can use emplace hint
    joints_[parent_id].children.emplace_hint(joints_[parent_id].children.end());

    return id;
  }

  /// Swaps the links at index a and index b
  /// Note:
  void swap(const size_t a, const size_t b) {
    if (a == b)
      return;

    const auto & link_a = joints_[a];
    const auto & link_b = joints_[b];

    // Swap parents
    auto & parent_a = joints_[link_a.parent];
    auto & parent_b = joints_[link_b.parent];
    // Remove from old parents
    parent_a.remove_child(a);
    parent_b.remove_child(b);
    // Add to new parents
    parent_b.children.emplace(a);
    parent_a.children.emplace(b);

    // Fix children
    for (const auto child : link_a.children)
      joints_[child].parent = b;
    for (const auto child : link_b.children)
      joints_[child].parent = a;

    // swap the actual data
    joints_.swap(a, b);
  }

  /// Links with a non-fixed parent joint from the URDF, named after the parent joint
  /// In my model, I treat these as the same thing. The link will have a frame in frames_ relative to the joint with an
  /// Eigen::Isometry3d::Identity origin.
  NameToVector<Joint> joints_{};

  /// Links from the URDF
  NameToVector<Frame> frames_{};
};

}

#endif //ARM_KINEMATICS_TREEORDERING_HPP
