//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP
#define ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP

#include <arm_kinematics/visibility_control.h>
#include <string>
#include <vector>
#include <urdf/model.h>
#include "kinematics_base.hpp"
#include <arm_kinematics/joint_map/joint_map_builder.hpp>
#include <arm_kinematics/aliases.hpp>
#include <arm_kinematics/frame_definitions.hpp>


namespace arm_kinematics {

/**
 * Plugin class used to perform forward kinematics.
 *
 * Has a good default implementation, but affords modification/extension/optimisation through pluginlib.
 */
class ARM_KINEMATICS_PUBLIC ForwardKinematicsPlugin : public KinematicsBase {
public:
  using SharedPtr = std::shared_ptr<ForwardKinematicsPlugin>;

  /**
   * \brief Abstract Base Class for the trees produced by FK plugins, which map joint states to Eigen::Isometry3d
   * transforms for different linkages.
   *
   * It is not responsible for modelling transmissions, and uses JointMaps for responsible.
   *
   * \note Chains were used for this job rather than the FK plugin directly, as to support complex use cases like
   * collision checking where the transforms of many colliders on the robot need to be updated.
   *
   * \note for implementing analytical Forward Kinematics -- Suppose you wanted to implement special case FK chains, you
   * can make your ForwardKinematicsPlugin inherit from the KDL implementation, then have make_chain(...) return
   * special implementations of Chain for special cases (such as when only the end effector is requested), and the
   * parent class implementation of make_chain otherwise.
   */
  class Tree {
  public:
    using SharedPtr = std::shared_ptr<Tree>;
    virtual ~Tree() = default;

    /**
     * Maps joint states to link poses.
     * \param[in]  joint_states The current positions of each joint
     * \param[out] link_poses The transform of each link requested in make_chain
     *
     * \warning inputs and outputs must be pre-allocated to the correct size!
     * \warning inputs and outputs must not point to the same memory, or be any of the class's internal vectors.
     */
    virtual void position_fk(const std::vector<double> & joint_states, Isometry3dVector & link_poses) = 0;

  protected:
    explicit Tree(const size_t link_count) : link_count(link_count) {}
    const size_t link_count = 0;
  };

  /**
   * \brief Constructs the plugin implementation's Tree subclass.
   * \see ForwardKinematicsPlugin::Tree
   *
   * \param joint_names[in] The name of the joints to use as inputs
   * \param base_link_name[in] The name of the frame to act as the origin
   * \param frames[in] The names and offsets from links to calculate Eigen::Isometry3d values for in \c Tree::position_fk().
   * You can also just wrap a string or vector of strings in {} if you don't care about the origin from FrameDefinitions
   * \param joint_map_builder[in] The builder used to construct the joint map needed for
   * \returns a chain that you can call .map() on with joint positions to get the poses for all the frames defined in
   * frames.
   *
   * \warning Likely very expensive, and obviously not real-time safe.
   * \warning Assume the parent ForwardKinematicsPlugin must stay alive for the lifetime of any chains it produces,
   * chain implementations may reference memory from the parent.
   */
  virtual Tree::SharedPtr make_tree(
    const std::vector<std::string> & joint_names,
    const std::string & base_link_name,
    FrameDefinitions frames,  //< You can std::move() here
    const JointMapBuilder & joint_map_builder) = 0;

  /**
   * Effectively replaces the constructor for the class, as we can only use a default constructor in plugins.
   *
   * \warning Very expensive, and obviously not real-time safe.
   * \returns True if initialization was successful. False otherwise.
   */
  bool initialize(
    KinematicsNodeInterfaces node_interfaces,
    std::string & robot_description,
    const std::vector<std::string>& joint_names = {},
    KinematicsParams kinematics_params = {});

  /**
   * Gets the default joint map builder used for constructing chains
   */
  [[nodiscard]] virtual const JointMapBuilder & get_joint_map_builder() const noexcept;
  [[nodiscard]] const urdf::Model & get_urdf_model() const noexcept;

protected:
  /**
   * Called when the kinematics plugin is created. Override this to add any set up logic for the kinematics plugin.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize() = 0;

private:
  urdf::Model urdf_model_;
  JointMapBuilder joint_map_builder_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP
