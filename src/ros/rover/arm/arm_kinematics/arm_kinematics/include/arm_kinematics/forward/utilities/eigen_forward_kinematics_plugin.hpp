//
// Created by Bailey Chessum on 17/11/2025.
//

#ifndef ARM_KINEMATICS_EIGEN_FORWARD_KINEMATICS_PLUGIN_HPP
#define ARM_KINEMATICS_EIGEN_FORWARD_KINEMATICS_PLUGIN_HPP

#include <arm_kinematics/forward/forward_kinematics_plugin.hpp>
#include <arm_kinematics/forward/utilities/compute_frame_tree.hpp>
#include <arm_kinematics/forward/utilities/analysis_tree.hpp>
#include <utility>
#include <arm_kinematics/visibility_control.h>

namespace arm_kinematics {

class ARM_KINEMATICS_PUBLIC EigenForwardKinematicsPlugin : public ForwardKinematicsPlugin {
public:
  /**
   * ForwardKinematicsPlugin::Tree implementation using ComputeFrameTree
   */
  class TreeImpl final : public Tree {
  public:
    using SharedPtr = std::shared_ptr<TreeImpl>;

    TreeImpl(const size_t output_count, ComputeFrameTree mapper, JointMap joint_map)
    : Tree(output_count),
      tree_(std::move(mapper)),
      joint_map_(std::move(joint_map)),
      mapped_joint_states_(joint_map_.output_count)
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
    void position_fk(const std::vector<double> & joint_states, Isometry3fVector & link_poses) override {
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
    std::vector<float> mapped_joint_states_{};
  };

  /**
   * \copydoc ForwardKinematicsPlugin::make_tree
   */
  MakeTreeResult make_tree(
    const std::vector<std::string> & joint_names,
    const std::string & base_link_name,
    const FrameDefinitions & frames,
    const JointMapBuilder & joint_map_builder) override;

  bool on_initialize() override;

private:
  AnalysisTree tree_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_EIGEN_FORWARD_KINEMATICS_PLUGIN_HPP