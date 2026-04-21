//
// Created by Bailey Chessum on 17/11/2025.
//

#ifndef ARM_KINEMATICS_DEFAULT_FORWARD_KINEMATICS_PLUGIN_HPP
#define ARM_KINEMATICS_DEFAULT_FORWARD_KINEMATICS_PLUGIN_HPP

#include <arm_kinematics/forward/forward_kinematics_plugin.hpp>
#include <arm_kinematics/forward/utilities/compute_frame_tree.hpp>
#include <arm_kinematics/forward/utilities/analysis_tree.hpp>
#include <arm_kinematics/joint_map/default_joint_map_builder.hpp>
#include <memory>
#include <mutex>
#include <utility>
#include <arm_kinematics/visibility_control.h>

namespace arm_kinematics {

/**
 * Default `ForwardKinematicsPlugin` implementation. Builds a `ComputeFrameTree` from the
 * robot's URDF + a runtime `JointMap` from the FK plugin's `DefaultJointMapBuilder`. The
 * "default" naming reflects that this is the canonical implementation shipped with the
 * package; the underlying math is currently Eigen-based but that's an implementation detail
 * users shouldn't pin to.
 */
class ARM_KINEMATICS_PUBLIC DefaultForwardKinematicsPlugin : public ForwardKinematicsPlugin {
public:
  /**
   * ForwardKinematicsPlugin::Tree implementation using ComputeFrameTree
   */
  class TreeImpl final : public Tree {
  public:
    using SharedPtr = std::shared_ptr<TreeImpl>;
    using UniquePtr = std::unique_ptr<TreeImpl>;

    TreeImpl(const size_t output_count, ComputeFrameTree mapper, JointMap joint_map)
    : Tree(output_count),
      tree_(std::move(mapper)),
      joint_map_(std::move(joint_map)),
      mapped_joint_states_(joint_map_.output_count())
    {
    }

    /**
     * Maps joint states to link poses.
     * \param[in]  joint_states The current positions of each joint
     * \param[out] link_poses The transform of each link requested in make_chain
     *
     * \warning inputs and outputs must be pre-allocated to the correct size!
     * \warning inputs and outputs must not point to the same memory, or be any of the class's internal vectors.
     */
    void position_fk(span<const double> joint_states, Isometry3dVector & link_poses) override {
      // Map to the joint state ordering decided by the out ref from build_fk_mapper_from_urdf
      joint_map_.map(joint_states, mapped_joint_states_);

      // Map those joint states to frames
      tree_.update(mapped_joint_states_, link_poses.data());
    }

    [[nodiscard]] const auto & get_tree() const noexcept { return tree_; }  //< For access in tests
    [[nodiscard]] const auto & get_joint_map() const noexcept { return joint_map_; }  //< For access in tests
    [[nodiscard]] const auto & get_mapped_joint_states() const noexcept { return mapped_joint_states_; }  //< For access in tests

  private:
    ComputeFrameTree tree_;
    JointMap joint_map_;
    std::vector<double> mapped_joint_states_{};
  };

  /**
   * \copydoc ForwardKinematicsPlugin::make_tree(span<const StateInterfaceDefinition>, ...)
   */
  tl::expected<MakeTreeResult, MakeTreeError> make_tree(
    span<const StateInterfaceDefinition> input_state_interfaces,
    const std::string & base_link_name,
    const FrameDefinitions & frames,
    const JointMapBuilder & joint_map_builder) override;

  // Bring the convenience overloads from the base class into scope so users can call them
  // without explicit name qualification.
  using ForwardKinematicsPlugin::make_tree;

  /**
   * \brief Returns this plugin's `DefaultJointMapBuilder`, lazy-constructed against
   * `get_transmission_analysis()` on the first call.
   *
   * \warning Relies on the implicit pointer-stability of the underlying analysis: this plugin
   * captures a reference on the first call and never re-checks. The default
   * `get_transmission_analysis()` delegates to `RobotModel::get_default_transmission_analysis()`,
   * whose returned reference is stable for the lifetime of the `RobotModel`.
   */
  [[nodiscard]] const JointMapBuilder & get_joint_map_builder() const noexcept override;

  bool on_initialize() override;

private:
  // One-shot lazy init of the default joint map builder. `mutable` so the lazy init can run
  // from a const accessor; `std::once_flag` makes it thread-safe and zero-cost on the hot path
  // after the first call.
  mutable std::once_flag joint_map_builder_once_{};
  mutable std::unique_ptr<DefaultJointMapBuilder> joint_map_builder_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_DEFAULT_FORWARD_KINEMATICS_PLUGIN_HPP
