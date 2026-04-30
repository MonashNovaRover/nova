//
// Created by Bailey Chessum on 19/11/2025.
//

#ifndef ARM_KINEMATICS_TREEORDERING_HPP
#define ARM_KINEMATICS_TREEORDERING_HPP

#include <vector>
#include <string>
#include <map>
#include <optional>
#include <rclcpp/logging.hpp>
#include <urdf/model.h>

#include "arm_kinematics/visibility_control.h"
#include "arm_kinematics/forward/frame_definitions.hpp"
#include "arm_kinematics/utilities/order.hpp"
#include "arm_kinematics/utilities/span.hpp"
#include "arm_kinematics/forward/utilities/joint_type.hpp"
#include "arm_kinematics/forward/utilities/compute_frame_tree.hpp"
#include "arm_kinematics/forward/utilities/compute_joint_tree.hpp"
#include "arm_kinematics/utilities/expected.hpp"
#include "arm_kinematics/utilities/to_eigen.hpp"

namespace arm_kinematics {

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
class ARM_KINEMATICS_PUBLIC AnalysisTree {
public:
  using JointId = size_t;
  using FrameId = size_t;

  struct JointQuery
  {
    Eigen::Isometry3d root_T_joint = Eigen::Isometry3d::Identity();
    Eigen::Vector3d axis_in_joint = Eigen::Vector3d::Zero();
    JointType type = JointType::CONTINUOUS;
  };

  /**
   * Data to describe a joint
   */
  struct JointDescription {
    JointType type = JointType::CONTINUOUS;
    Eigen::Vector3d axis = Eigen::Vector3d::Zero();

    JointDescription() = default;
    explicit JointDescription(const urdf::Joint & joint)
    : type(get_type(joint).value_or(JointType::CONTINUOUS)), axis(to_eigen(joint.axis)) {}
    explicit JointDescription(const urdf::Joint * joint)
    {
      *this = joint ? JointDescription(*joint) : JointDescription();
    }

    [[nodiscard]] JointDescription reversed() const noexcept {
      auto reversed_joint = JointDescription();
      reversed_joint.type = type;
      reversed_joint.axis = -axis;
      return reversed_joint;
    }
  };

  /**
   * Defines a link with a joint in the tree. Fixed links are considered to be 'frames'.
   */
  struct Joint
  {
    /// ID of the frame that this link actuates relative to.
    /// If 0, it is relative to the root. The root will have itself as the parent.
    JointId parent = 0; //< 0 is the root, and is always a dummy
    /// Distance from the root node (id = 0)
    size_t depth = 0;

    /// The type represented by this joint. Fixed if std::nullopt
    JointDescription joint{};
    /// Defines the reference frame of the link relative to the parent
    Eigen::Isometry3d origin = Eigen::Isometry3d::Identity();

    /// The IDs of all other joints that actuate relative to this link. All elements of this set should be larger than
    /// this link's id.
    std::vector<JointId> children{};
    /// The ids of all frames that point to this handle
    std::vector<FrameId> frames{};

    /**
     * Update internal indices to match the given order.
     * @param order The order to be applied
     */
    void sort(const Order<> & order) {
      parent = order.inverse[parent];

      children = order.inverse * children;
    }
  };

  /**
   * A non-actuated link relative to some actuated link
   */
  struct Frame
  {
    /// The ID of the joint this frame is relative to
    JointId parent = 0;
    /// The transform from the parent link to this frame
    Eigen::Isometry3d origin;
  };

  explicit AnalysisTree() {
    // Only include dummy root
    joints_.reserve(1);
    joint_name_order_.set("", 0);
    joints_.push_back({});
  }

  explicit AnalysisTree(const urdf::Model & model)
  {
    // Reserve joints_, assume 1 link for each non-fixed joint
    size_t joint_count = 0;
    for (const auto & [name, joint] : model.joints_)
      if (joint->type != urdf::Joint::FIXED)  //< Our tree model does not consider fixed joints to be joints
        joint_count++;
    joints_.reserve(joint_count + 1);  //< +1 for the dummy root
    // Add the dummy root
    joint_name_order_.set("", 0);
    joints_.push_back({});

    // Create frames for each link in model
    frames_.reserve(model.links_.size());
    for (const auto & [name, link] : model.links_)
      add_frame(name, to_frame(link));
  }

  explicit AnalysisTree(
    const AnalysisTree & other,
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

  JointId add_joint(
    const std::string & name,
    const JointId parent_id,
    const JointDescription & joint_description,
    const Eigen::Isometry3d & origin)
  {
    const JointId id = joints_.size();
    joint_name_order_.set(name, id);
    joints_.push_back({parent_id, joints_[parent_id].depth + 1, joint_description, origin});
    joints_[parent_id].children.emplace_back(id);  //< re-index after push_back; id is current largest
    return id;
  }

  FrameId add_frame(
    const std::string & name,
    const FrameId parent_id,
    const Eigen::Isometry3d & origin)
  {
    if (parent_id >= joints_.size())
      throw std::runtime_error(std::to_string(parent_id) + " (parent_id) >= (joints_.size()) " + std::to_string(joints_.size()));
    assert(parent_id < joints_.size());
    const FrameId id = frames_.size();
    frame_name_order_.set(name, id);
    frames_.push_back({parent_id, origin});
    joints_[parent_id].frames.push_back(id);  //< id is current largest, and will be last
    return id;
  }

  [[nodiscard]] std::vector<size_t> get_subtree_joint_count() const noexcept {
    std::vector<size_t> subtree_sizes(joints_.size(), 1);

    for (size_t i = joints_.size() - 1; i != static_cast<size_t>(-1); --i) {
      const auto & joint = joints_[i];

      for (const auto & child_id : joint.children) {
        subtree_sizes[i] += subtree_sizes[child_id];
      }
    }
    subtree_sizes[0] = joints_.size();

    return subtree_sizes;
  }

  [[nodiscard]] std::vector<size_t> get_subtree_joint_branch_count() const noexcept {
    std::vector<size_t> branch_count(joints_.size(), 0);

    for (size_t i = joints_.size() - 1; i > 0; ++i) {
      const auto & joint = joints_[i];

      for (const auto & child_id : joint.children) {
        branch_count[i] += branch_count[child_id];
      }
    }
    branch_count[0] = joints_.size();

    return branch_count;
  }

  void sort_joints(const Order<> & order) noexcept {
    assert(order[0] == 0);

    joints_ = order.reorder(joints_);
    joint_name_order_.sort(order);
    for (auto & joint : joints_) {
      joint.sort(order);
    }

    for (auto & frame : frames_) {
      frame.parent = order.inverse[frame.parent];
    }
  }

  Order<> sort_joints(bool sort_children = true);

  Order<> sort_frames() noexcept;

  void log(rclcpp::Logger logger) {
    const auto & jno = joint_name_order_;  // const ref so operator[] returns const std::string&

    std::stringstream ss{};
    ss << "Tree:";
    for (size_t i = 0; i < joints_.size(); ++i) {
      const auto & joint = joints_[i];

      ss << "\n";
      ss << "[" << std::to_string(i) << "] \"" << jno.inverse[i] << "\"\t-> ";

      for (const auto & child : joint.children) {
        if (child < jno.size())
          ss << "\"" << jno.inverse[child] << "\" (" << std::to_string(child) << "), ";
        else
          ss << "<invalid> (" << std::to_string(child) << "), ";
      }
    }

    RCLCPP_INFO(logger, "%s", ss.str().c_str());
  }

  /**
   *
   * @returns the number of leaf children in children
   */
  void sort_joint_children_strategy_a();

  /**
   * You need to have sorted joints then frames before calling.
   */
  tl::expected<ComputeFrameTree, std::string_view> make_compute_frame_tree();

  /**
   * You need to have sorted joints before calling.
   */
  ComputeJointTree make_compute_joint_tree();

  [[nodiscard]] JointQuery query_joint(JointId joint_id) const;
  [[nodiscard]] JointQuery query_joint(const std::string & joint_name) const;

  [[nodiscard]] Eigen::Isometry3d query_frame(FrameId frame_id) const;
  [[nodiscard]] Eigen::Isometry3d query_frame(const std::string & frame_name) const;

  [[nodiscard]] Eigen::Isometry3d query_transform_between_frames(
    FrameId from_frame_id,
    FrameId to_frame_id) const;
  [[nodiscard]] Eigen::Isometry3d query_transform_between_frames(
    const std::string & from_frame_name,
    const std::string & to_frame_name) const;

  // Accessors
  [[nodiscard]] const std::vector<Joint> & get_joints() const noexcept { return joints_; }
  [[nodiscard]] const std::vector<Frame> & get_frames() const noexcept { return frames_; }
  [[nodiscard]] const Order<std::string, JointId> & joint_name_order() const noexcept { return joint_name_order_; }
  [[nodiscard]] const Order<std::string, FrameId> & frame_name_order() const noexcept { return frame_name_order_; }
  /// Returns a span over all joint names in id order, including the dummy root at index 0.
  [[nodiscard]] span<const std::string> joint_names() const noexcept {
    return {joint_name_order_.inverse.data(), joint_name_order_.inverse.size()};
  }

private:
  /// Helper used by the URDF constructor's add_frame overload that takes a Frame directly.
  FrameId add_frame(const std::string & name, const Frame & frame)
  {
    return add_frame(name, frame.parent, frame.origin);
  }

  [[nodiscard]] Isometry3dVector compute_root_to_joint_poses() const;

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
  JointId find_or_create_joint_link(urdf::Link const * child_link)
  {
    if (!child_link || !child_link->parent_joint)
      return 0; //< dummy root

    assert(child_link->parent_joint->type != urdf::Joint::FIXED);

    // Check for existing construction.
    if (joint_name_order_.contains_key(child_link->parent_joint->name))
      return joint_name_order_[child_link->parent_joint->name];

    // To keep in topological order, we must ensure the parent exists first
    auto origin = to_eigen(child_link->parent_joint->parent_to_joint_origin_transform);
    const auto grandparent_joint = find_next_non_fixed_joint(child_link->getParent().get(), origin); //< may be nullptr!
    const JointId parent_id = find_or_create_joint_link(grandparent_joint);                          //< handles nullptr

    // Create new link
    const JointId id = joints_.size();
    joint_name_order_.set(child_link->parent_joint->name, id);
    joints_.push_back({
      parent_id,
      joints_[parent_id].depth + 1,
      JointDescription(*child_link->parent_joint),
      origin,
    });

    // Register as child of parent
    assert(id > parent_id);
    // We know id will be last, so we can use emplace hint
    joints_[parent_id].children.emplace_back(id);

    return id;
  }

  /// Links with a non-fixed parent joint from the URDF, named after the parent joint.
  /// Indexed by JointId. Joint 0 is always the dummy root.
  std::vector<Joint> joints_{};
  Order<std::string, JointId> joint_name_order_{};

  /// Links from the URDF. Indexed by FrameId.
  std::vector<Frame> frames_{};
  Order<std::string, FrameId> frame_name_order_{};

  bool sorted_joints_ = false;
  bool sorted_frames_ = false;
};

}

#endif //ARM_KINEMATICS_TREEORDERING_HPP
