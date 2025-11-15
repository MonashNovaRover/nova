//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP
#define ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP

#include <arm_kinematics/visibility_control.h>
#include <string>
#include <vector>
#include <Eigen/Geometry>
#include <urdf/model.h>
#include <kdl_parser/kdl_parser.hpp>
#include <kdl/jntarray.hpp>
#include "kinematics_base.hpp"
#include <arm_kinematics/joint_map/joint_map.hpp>
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
   * \brief Abstract Base Class for the chains produced by FK plugins, which map joint states to Eigen::Isometry3d
   * transforms for different linkages.
   *
   * It is not responsible for modelling transmissions, and uses JointMaps for responsible.
   *
   * \note Chains were used for this job rather than the FK plugin directly, as to support complex use cases like
   * collision checking where the transforms of many colliders on the robot need to be updated.
   *
   * \note for implementing analytical Forward Kinematics -- Suppose you wanted to implement special case FK chains, you
   * can can make your ForwardKinematicsPlugin inherit from the KDL implementation, then have make_chain(...) return
   * special implementations of Chain for special cases (such as when only the end effector is requested), and the
   * parent class implementation of make_chain otherwise.
   *
   * TODO: Mechanism to reuse FK Calculations (for example, in hot loops where you need both collision checking chains
   *       and the end effector pose, which is already included as part of the collision checking FK).
   */
  class Chain {
  public:
    using SharedPtr = std::shared_ptr<Chain>;

    /**
     * Maps joint states to link poses.
     * \param[in]  joint_states The current positions of each joint
     * \param[out] link_poses The transform of each link requested in make_chain
     *
     * \warning inputs and outputs must be pre-allocated to the correct size!
     * \warning inputs and outputs must not point to the same memory, or be any of the class's internal vectors.
     */
    virtual void position_fk(const std::vector<double> & joint_states, Isometry3dVector & link_poses) = 0;

    explicit Chain(const size_t link_count) : link_count(link_count) {}

    const size_t link_count = 0;
  };

  /**
   * \brief Constructs the plugin implementation's Chain subclass.
   * \see ForwardKinematicsPlugin::Chain
   *
   * \param joint_names[in] The name of the joints to use as inputs
   * \param frames[in] The names and offsets from links to calculate Eigen::Isometry3d values for in \c Chain::position_fk().
   * You can also just wrap a string or vector of strings in {} if you don't care about the origin from FrameDefinitions
   * \param joint_map_builder[in] The builder used to construct the joint map needed for
   * \returns a chain that you can call .map() on with joint positions to get the poses for all the frames defined in
   * frames.
   *
   * \warning Likely very expensive, and obviously not real-time safe.
   * \warning Assume the parent ForwardKinematicsPlugin must stay alive for the lifetime of any chains it produces,
   * chain implementations may reference memory from the parent.
   */
  virtual Chain::SharedPtr make_chain(
    const std::vector<std::string> & joint_names,
    FrameDefinitions frames,  //< You can std::move() here
    const JointMapBuilder & joint_map_builder) = 0;

  /**
   * Effectively replaces the constructor for the class, as we can only use a default constructor in plugins.
   *
   * \warning Very expensive, and obviously not real-time safe.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  bool initialize(
    KinematicsNodeInterfaces node_interfaces,
    std::string & robot_description,
    const std::vector<std::string>& joint_names = {},
    KinematicsParams kinematics_params = {});

  /**
   * Do Forward Kinematics to find the position of a link with the given name.
   *
   * \param[in] joint_angles The current angle of every joint in get_joint_names(), in radians.
   * \param[in] link_name The name of the link to find the pose for.
   * \param[out] solution_pose The pose found through forward kinematics.
   *
   * \note Make sure to pre-allocate vectors outside of the real-time loop with the correct number of elements (same
   * number of joints as in joint names).
   *
   * \returns True if a solution could be found. False otherwise.
   */
  // virtual bool get_position_fk(
  //   const std::vector<double> & joint_angles,
  //   const JointMap & joint_map,
  //   const KDL::Chain & kdl_chain,
  //   KDL::JntArray & kdl_chain_jnts,
  //   Eigen::Isometry3d & solution_pose) const;

  /**
    * Do Forward Kinematics to find the position of the main End Effector link.
    *
    * \param[in] joint_angles The current angle of every joint in get_joint_names(), in radians.
    * \param[out] solution_pose The pose found through forward kinematics.
    *
    * \note Make sure to pre-allocate vectors outside of the real-time loop with the correct number of elements (same
    * number of joints as in joint names).
    *
    * \returns True if a solution could be found. False otherwise.
    */
  virtual bool get_position_fk(
    const std::vector<double> & joint_angles,
    Eigen::Isometry3d & solution_pose);

  /**
   * Gets the default joint map builder used for constructing chains
   */
  [[nodiscard]] virtual const JointMapBuilder & get_joint_map_builder() const noexcept;
  [[nodiscard]] const urdf::Model & get_urdf_model() const noexcept;
  [[nodiscard]] const KDL::Tree & get_kdl_tree() const noexcept;
  [[nodiscard]] const KDL::Chain & get_kdl_chain() const noexcept;
  [[nodiscard]] const JointMap & get_kdl_chain_joint_map() const noexcept;

protected:
  /**
   * Called when the kinematics plugin is created. Override this to add any set up logic for the kinematics plugin.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize() = 0;

private:
  // KDL
  urdf::Model urdf_model_;
  KDL::Tree kdl_tree_;
  /// The default kdl chain to use, from the base link to the ee link
  KDL::Chain kdl_chain_;
  /// Pre-allocated KDL::JntArray to use for per-joint values
  KDL::JntArray preallocated_jnts_{};

  JointMapBuilder joint_map_builder_{};

  /// Maps joint values from joint_names to the joint values needed by the kdl_chain_ when doing forward kinematics
  JointMap chain_joint_map_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP
