//
// Created by Bailey Chessum on 19/11/2025.
//

#ifndef ARM_KINEMATICS_TREEORDERING_HPP
#define ARM_KINEMATICS_TREEORDERING_HPP

#include <vector>
#include <string>
#include <map>
#include <optional>
#include <set>
#include <rclcpp/logging.hpp>
#include <urdf/model.h>
#include "name_to_vector.hpp"
#include <arm_kinematics/frame_definitions.hpp>
#include <arm_kinematics/utilities/order.hpp>
#include <arm_kinematics/forward/eigen/joint_type.hpp>

#include <arm_kinematics/forward/eigen/compute_frame_tree.hpp>
#include <arm_kinematics/forward/eigen/compute_joint_tree.hpp>
#include <arm_kinematics/utilities/expected.hpp>

namespace arm_kinematics {

/**
 * Gets the equivalent ComputeJointTree type for a URDF joint
 * \param joint The joint
 * \return std::nullopt, or a joint type
 */
inline std::optional<JointType> get_type(const urdf::JointConstSharedPtr & joint) {
  switch (joint->type) {
    case urdf::Joint::REVOLUTE:
      return JointType::REVOLUTE;
    case urdf::Joint::PRISMATIC:
      return JointType::PRISMATIC;
    case urdf::Joint::CONTINUOUS:
      return JointType::CONTINUOUS;
    default:
      return std::nullopt;
  }
}

/**
 * Gets the equivalent ComputeJointTree type for a URDF joint
 * \param joint The joint
 * \return std::nullopt, or a joint type
 */
inline std::optional<JointType> get_type(const urdf::Joint & joint) {
  switch (joint.type) {
  case urdf::Joint::REVOLUTE:
    return JointType::REVOLUTE;
  case urdf::Joint::PRISMATIC:
    return JointType::PRISMATIC;
  case urdf::Joint::CONTINUOUS:
    return JointType::CONTINUOUS;
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
    size_t parent = 0; //< 0 is the root, and is always a dummy
    /// Distance from the root node (id = 0)
    size_t depth = 0;

    /// The type represented by this joint. Fixed if std::nullopt
    JointDescription joint{};
    /// Defines the reference frame of the link relative to the parent
    Eigen::Isometry3d origin = Eigen::Isometry3d::Identity();

    /// The IDs of all other frames that actuate relative to this link. All elements of this set should be larger than
    /// this link's id.
    std::vector<size_t> children{};
    /// The ids of all frames that point to this handle
    std::vector<size_t> frames{};

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
    /// The ID of the link this is relative
    size_t parent = 0;
    /// The transform from the parent link to this frame
    Eigen::Isometry3d origin;
  };

  explicit AnalysisTree() {
    // Only include dummy root
    joints_.reserve(1);
    joints_.add("", {});
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
    joints_.add("", {});

    // Create frames for each link in model
    frames_.reserve(model.links_.size());
    for (const auto & [name, link] : model.links_)
      frames_.add(name, to_frame(link));
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

  size_t add_joint(
    const std::string & name,
    const size_t parent_id,
    const JointDescription & joint_description,
    const Eigen::Isometry3d & origin)
  {
    auto & parent = joints_[parent_id];

    size_t id = joints_.add(name, {
      parent_id,
      joints_[parent_id].depth + 1,
      joint_description,
      origin
    });

    // Add as child of parent
    parent.children.emplace_back(id);  //< id is current largest, and will be last

    return id;
  }

  size_t add_frame(
    const std::string & name,
    const size_t parent_id,
    const Eigen::Isometry3d & origin)
  {
    const size_t id = frames_.add(name, {
      parent_id,
      origin
    });

    // Add as child of parent
    if (parent_id >= joints_.size())
      throw std::runtime_error(std::to_string(parent_id) + " (parent_id) >= (joints_.size()) " + std::to_string(joints_.size()));
    assert(parent_id < joints_.size());
    auto & parent = joints_[parent_id];
    parent.frames.push_back(id);  //< id is current largest, and will be last

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

    joints_.sort(order);
    for (auto & joint : joints_.data) {
      joint.sort(order);
    }

    for (auto & frame : frames_.data) {
      frame.parent = order.inverse[frame.parent];
    }
  }

  Order<> sort_joints(bool sort_children = true);

  Order<> sort_frames() noexcept;

  void log(rclcpp::Logger logger) {

    std::stringstream ss{};
    ss << "Tree:";
    for (size_t i = 0; i < joints_.size(); ++i) {
      const auto & joint = joints_[i];
      const auto & name = joints_.names[i];

      ss << "\n";
      ss << "[" << std::to_string(i) << "] \"" << name << "\"\t-> ";

      for (const auto & child : joint.children) {
        if (child < joints_.names.size())
          ss << "\"" << joints_.names[child] << "\" (" << std::to_string(child) << "), ";
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

    RCLCPP_INFO(rclcpp::get_logger("analysis_tree"), "Creating joint %s at link %s",
      child_link->parent_joint->name.c_str(), child_link->name.c_str());

    assert(child_link->parent_joint->type != urdf::Joint::FIXED);

    // Check for existing construction.
    if (joints_.contains(child_link->parent_joint->name))
      return joints_[child_link->parent_joint->name];

    // To keep in topological order, we must ensure the parent exists first
    auto origin = to_eigen(child_link->parent_joint->parent_to_joint_origin_transform);
    const auto grandparent_joint = find_next_non_fixed_joint(child_link->getParent().get(), origin); //< may be nullptr!
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
    joints_[parent_id].children.emplace_back(id);

    return id;
  }

  /// Links with a non-fixed parent joint from the URDF, named after the parent joint
  /// In my model, I treat these as the same thing. The link will have a frame in frames_ relative to the joint with an
  /// Eigen::Isometry3d::Identity origin.
  NameToVector<Joint> joints_{};

  /// Links from the URDF
  NameToVector<Frame> frames_{};

  bool sorted_joints_ = false;
  bool sorted_frames_ = false;
};

}

#endif //ARM_KINEMATICS_TREEORDERING_HPP
