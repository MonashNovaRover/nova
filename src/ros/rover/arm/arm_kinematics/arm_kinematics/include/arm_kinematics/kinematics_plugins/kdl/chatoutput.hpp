// eigen_tree_forward_kinematics_plugin.hpp

#pragma once

#include <arm_kinematics/forward_kinematics_plugin.hpp>  // your header with ForwardKinematicsPlugin etc.
#include <urdf/model.h>
#include <Eigen/Geometry>
#include <unordered_map>

/**
 * A ForwardKinematicsPlugin implementation that builds an index-based URDF tree
 * once, and uses Eigen::Isometry3d to compute FK with no allocations in the
 * real-time FK loop.
 */
class EigenTreeForwardKinematicsPlugin : public ForwardKinematicsPlugin {
public:
  EigenTreeForwardKinematicsPlugin() = default;
  ~EigenTreeForwardKinematicsPlugin() override = default;

  /**
   * Builds the internal URDF tree model (index-based).
   * Called by ForwardKinematicsPlugin::initialize().
   */
protected:
  bool on_initialize() override {
    const urdf::Model & model = get_urdf_model();

    auto root_link = model.root_link_;
    if (!root_link) {
      RCLCPP_ERROR(get_logger(), "URDF model has no root link");
      return false;
    }

    link_nodes_.clear();
    link_index_by_name_.clear();
    active_joint_names_.clear();

    root_index_ = -1;
    buildFromUrdfRecursive(root_link, -1);

    if (root_index_ < 0) {
      RCLCPP_ERROR(get_logger(), "Failed to identify root index in FK plugin");
      return false;
    }

    return true;
  }

public:
  class EigenTreeChain;

  Chain::SharedPtr make_chain(
    const std::vector<std::string> & joint_names,
    const std::string & base_link_name,
    FrameDefinitions frames,
    const JointMapBuilder & joint_map_builder) override;

private:
  enum class JointType {
    FIXED,
    REVOLUTE,
    PRISMATIC,
    CONTINUOUS
  };

  struct LinkNode {
    int parent_index = -1;                     // index into link_nodes_, -1 for root
    int joint_index  = -1;                     // index into active_joint_names_, or -1 if fixed
    JointType joint_type = JointType::FIXED;   // joint type

    Eigen::Vector3d axis = Eigen::Vector3d::Zero();   // joint axis (in joint frame)
    Eigen::Isometry3d parent_T_joint = Eigen::Isometry3d::Identity(); // parent link -> joint frame
    Eigen::Isometry3d joint_T_child  = Eigen::Isometry3d::Identity(); // joint frame -> child link frame

    std::string name;  // link name (for debugging / init only)
  };

  std::vector<LinkNode> link_nodes_;
  std::unordered_map<std::string, int> link_index_by_name_;
  std::vector<std::string> active_joint_names_; // All non-fixed joints, in index order
  int root_index_ = -1;

  /// Build link_nodes_ / link_index_by_name_ / active_joint_names_ recursively.
  void buildFromUrdfRecursive(const urdf::LinkConstSharedPtr & link, int parent_index) {
    const int this_index = static_cast<int>(link_nodes_.size());
    link_index_by_name_[link->name] = this_index;

    LinkNode node;
    node.parent_index = parent_index;
    node.name = link->name;

    if (!link->parent_joint) {
      // Root link: no motion relative to parent (world/model frame).
      node.joint_type = JointType::FIXED;
      node.joint_index = -1;
      node.parent_T_joint.setIdentity();
      node.joint_T_child.setIdentity();

      if (root_index_ < 0) {
        root_index_ = this_index;
      }
    } else {
      const auto & joint = link->parent_joint;

      // Convert origin (parent -> joint frame) to Eigen
      node.parent_T_joint = Eigen::Isometry3d::Identity();
      if (joint->parent_to_joint_origin_transform) {
        const auto & p = joint->parent_to_joint_origin_transform->position;
        const auto & r = joint->parent_to_joint_origin_transform->rotation;
        node.parent_T_joint.translation() = Eigen::Vector3d(p.x, p.y, p.z);

        Eigen::AngleAxisd Rx(r.roll,  Eigen::Vector3d::UnitX());
        Eigen::AngleAxisd Ry(r.pitch, Eigen::Vector3d::UnitY());
        Eigen::AngleAxisd Rz(r.yaw,   Eigen::Vector3d::UnitZ());
        node.parent_T_joint.linear() = (Rz * Ry * Rx).toRotationMatrix();
      }

      node.joint_T_child.setIdentity(); // URDF child link frame is after joint transform

      // Joint type
      switch (joint->type) {
        case urdf::Joint::REVOLUTE:
          node.joint_type = JointType::REVOLUTE;
          break;
        case urdf::Joint::CONTINUOUS:
          node.joint_type = JointType::CONTINUOUS;
          break;
        case urdf::Joint::PRISMATIC:
          node.joint_type = JointType::PRISMATIC;
          break;
        case urdf::Joint::FIXED:
        default:
          node.joint_type = JointType::FIXED;
          break;
      }

      // Axis
      if (joint->type != urdf::Joint::FIXED) {
        node.axis = Eigen::Vector3d(
          joint->axis.x,
          joint->axis.y,
          joint->axis.z
        );
        if (node.axis.norm() == 0.0) {
          node.axis = Eigen::Vector3d::UnitZ(); // fallback
        } else {
          node.axis.normalize();
        }
      } else {
        node.axis = Eigen::Vector3d::Zero();
      }

      // Only non-fixed joints get a DOF and appear in active_joint_names_
      if (node.joint_type == JointType::REVOLUTE ||
          node.joint_type == JointType::PRISMATIC ||
          node.joint_type == JointType::CONTINUOUS) {
        node.joint_index = static_cast<int>(active_joint_names_.size());
        active_joint_names_.push_back(joint->name);
      } else {
        node.joint_index = -1; // fixed => no DOF
      }
    }

    link_nodes_.push_back(node);

    // Recurse into children
    for (const auto & child : link->child_links) {
      buildFromUrdfRecursive(child, this_index);
    }
  }

public:
  /**
   * Accessors used by EigenTreeChain.
   */
  const std::vector<LinkNode> & links() const noexcept { return link_nodes_; }
  const std::vector<std::string> & activeJointNames() const noexcept { return active_joint_names_; }
  const std::unordered_map<std::string, int> & linkIndexByName() const noexcept { return link_index_by_name_; }
  int rootIndex() const noexcept { return root_index_; }
};


// ------------------ EigenTreeChain implementation ------------------

class EigenTreeForwardKinematicsPlugin::EigenTreeChain : public ForwardKinematicsPlugin::Chain {
public:
  EigenTreeChain(
    const EigenTreeForwardKinematicsPlugin & plugin,
    JointMap joint_map,
    int base_link_index,
    std::vector<int> frame_link_indices,
    Isometry3dVector frame_offsets)
  : Chain(frame_offsets.size())
  , plugin_(plugin)
  , joint_map_(std::move(joint_map))
  , base_link_index_(base_link_index)
  , frame_link_indices_(std::move(frame_link_indices))
  , frame_offsets_(std::move(frame_offsets))
  , joint_positions_(plugin.activeJointNames().size(), 0.0)
  , link_transforms_(plugin.links().size(), Eigen::Isometry3d::Identity())
  {
    assert(frame_link_indices_.size() == frame_offsets_.size());
  }

  /**
   * Real-time FK method: no allocations.
   *
   * joint_states: input positions in the user-specified order (matching the joint_names
   *               given when make_chain() was called).
   * link_poses:   output frames for each requested FrameDefinitions entry, relative
   *               to the base link.
   */
  void position_fk(const std::vector<double> & joint_states, Isometry3dVector & link_poses) override {
    assert(link_poses.size() == frame_offsets_.size());
    assert(joint_states.size() == joint_map_.input_count);
    assert(joint_positions_.size() == joint_map_.output_count);

    // 1) Map joints: user order / transmissions / mimics -> active_joint_names_ order
    joint_map_.map(joint_states, joint_positions_);

    // 2) Compute FK for all links in the robot (or at least the URDF tree we built)
    const auto & links = plugin_.links();
    const int root_idx = plugin_.rootIndex();

    // Root transform is identity (or world->root if you add that later)
    link_transforms_[root_idx].setIdentity();

    for (std::size_t idx = 0; idx < links.size(); ++idx) {
      const int i = static_cast<int>(idx);
      if (i == root_idx) {
        continue;
      }

      const auto & node = links[i];
      const auto & parent_T = link_transforms_[node.parent_index];

      // Start with parent -> joint transform
      Eigen::Isometry3d T = node.parent_T_joint;

      // Apply joint motion if this link has a DOF
      if (node.joint_index >= 0) {
        const double q = joint_positions_[static_cast<std::size_t>(node.joint_index)];
        Eigen::Isometry3d T_motion = Eigen::Isometry3d::Identity();

        switch (node.joint_type) {
          case JointType::REVOLUTE:
          case JointType::CONTINUOUS: {
            Eigen::AngleAxisd aa(q, node.axis);
            T_motion.linear() = aa.toRotationMatrix();
            break;
          }
          case JointType::PRISMATIC: {
            T_motion.translation() = node.axis * q;
            break;
          }
          case JointType::FIXED:
          default:
            // no-op
            break;
        }

        T = T * T_motion;
      }

      // Joint -> child link
      T = T * node.joint_T_child;

      // Compose with parent world transform
      link_transforms_[i] = parent_T * T;
    }

    // 3) Express requested frames relative to base_link_index_
    const Eigen::Isometry3d & T_world_base = link_transforms_[base_link_index_];
    const Eigen::Isometry3d T_base_world = T_world_base.inverse();

    for (std::size_t i = 0; i < frame_link_indices_.size(); ++i) {
      const int link_idx = frame_link_indices_[i];
      const Eigen::Isometry3d & T_world_link = link_transforms_[link_idx];

      // base -> link (frame origin)
      Eigen::Isometry3d T_base_link = T_base_world * T_world_link;

      // Include the requested offset origin
      link_poses[i] = T_base_link * frame_offsets_[i];
    }
  }

private:
  const EigenTreeForwardKinematicsPlugin & plugin_;

  JointMap joint_map_;                 // input -> active_joint_names_ mapping
  const int base_link_index_;          // link index of base_link_name used for this chain

  std::vector<int> frame_link_indices_; // link index per output frame
  Isometry3dVector frame_offsets_;      // per-frame offset in parent link frame

  std::vector<double> joint_positions_; // size = active_joint_names_.size()
  Isometry3dVector link_transforms_;    // size = links().size()
};


// ------------------ make_chain implementation ------------------

inline ForwardKinematicsPlugin::Chain::SharedPtr
EigenTreeForwardKinematicsPlugin::make_chain(
  const std::vector<std::string> & joint_names,
  const std::string & base_link_name,
  FrameDefinitions frames,
  const JointMapBuilder & joint_map_builder)
{
  // Build a JointMap that maps from the user-provided joint_names
  // to our active_joint_names_ (non-fixed joints only).
  const auto & output_names = activeJointNames();
  JointMap joint_map = joint_map_builder.build(joint_names, output_names);

  // Base link index
  const auto & link_index_map = linkIndexByName();
  auto base_it = link_index_map.find(base_link_name);
  if (base_it == link_index_map.end()) {
    throw std::runtime_error("Base link name not found in URDF: " + base_link_name);
  }
  const int base_idx = base_it->second;

  // Map each FrameDefinitions parent_link_name to a link index
  std::vector<int> frame_indices;
  frame_indices.reserve(frames.parent_link_names.size());

  for (const auto & name : frames.parent_link_names) {
    auto it = link_index_map.find(name);
    if (it == link_index_map.end()) {
      throw std::runtime_error("Frame parent link not found in URDF: " + name);
    }
    frame_indices.push_back(it->second);
  }

  // Construct the chain. All allocations happen here; position_fk is allocation-free.
  return std::make_shared<EigenTreeChain>(
    *this,
    std::move(joint_map),
    base_idx,
    std::move(frame_indices),
    std::move(frames.origins));
}
