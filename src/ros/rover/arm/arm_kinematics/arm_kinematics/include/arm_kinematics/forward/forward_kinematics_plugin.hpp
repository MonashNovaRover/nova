//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP
#define ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP

#include <arm_kinematics/visibility_control.h>
#include <string>
#include <vector>
#include <urdf/model.h>
#include <arm_kinematics/common/kinematics_base.hpp>
#include <arm_kinematics/joint_map/joint_map_builder.hpp>
#include <arm_kinematics/utilities/aliases.hpp>
#include <arm_kinematics/forward/frame_definitions.hpp>
#include <arm_kinematics/utilities/order.hpp>

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
   * \brief Abstract Base Class for the trees produced by FK plugins, which map joint states to Eigen::Isometry3f
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
    virtual void position_fk(const std::vector<double> & joint_states, Isometry3fVector & link_poses) = 0;

    // TODO: velocity_fk?

  protected:
    explicit Tree(const size_t link_count) : link_count(link_count) {}
    const size_t link_count = 0;
  };

  /**
   * When you create a Tree, the order in which you provide the requested frames might need to change. So, I can't just
   * provide you with the tree that you created, but also with information about how you might need to rearrange your
   * data to match the input order it needs. Hence, this struct exists. Sorry!
   *
   * Get data out with structured bindings:
   * \code
   *   auto [tree, order] = plugin->make_tree({"j1", "j2"}, "base_link", {"ee_link"});
   * \endcode
   */
  struct MakeTreeResult {
    /**
     * The FK tree pointer that you wanted to make.
     */
    Tree::SharedPtr tree;

    /**
     * The frames you requested when calling make_tree were rearranged. This encodes how it was rearranged.
     *
     * order[i] is the original index for the frame at the new index i.
     * order.inverse[i] is the new index for the frame originally at index i.
     *
     * You can shuffle an array to be in this new order (vectors are reordered my making a copy) by:
     * \code
     *   std::vector<std::string> my_vec;
     *   // ...
     *   my_vec = order.reorder(my_vec);
     * \endcode
     *
     * If you don't want to modify the original collection, the \c Reordered struct provides a helper for indexing into
     * your collection through the new \c order :
     * \code
     *   std::vector<std::string> my_original_vec;
     *   // ...
     *   auto my_reordered_vec = Reordered(my_original_vec, order);
     *   auto element = my_reordered_vec[0];  //< Index in, or loop over it, etc...
     * \endcode
     */
    Order<> order;
  };

  /**
   * \brief Constructs the plugin implementation's Tree subclass.
   * \see ForwardKinematicsPlugin::Tree
   *
   * \param joint_names[in] The name of the joints to use as inputs
   * \param base_link_name[in] The name of the frame to act as the origin
   * \param frames[in] The names and offsets from links to calculate Eigen::Isometry3f values for in \c Tree::position_fk().
   * You can also just wrap a string or vector of strings in {} if you don't care about the origin from FrameDefinitions
   * \param joint_map_builder[in] The builder used to construct the joint map needed for
   * \returns a tree that you can call .position_fk() on with joint positions to get the poses for all the frames defined in
   * frames.
   *
   * \warning Likely very expensive, and obviously not real-time safe.
   * \warning Assume the parent ForwardKinematicsPlugin must stay alive for the lifetime of any chains it produces,
   * chain implementations may reference memory from the parent.
   */
  virtual MakeTreeResult make_tree(
    const std::vector<std::string> & joint_names,
    const std::string & base_link_name,
    const FrameDefinitions & frames,  //< TODO: Maybe make pass by copy to allow for std::move where appropriate
    const JointMapBuilder & joint_map_builder) = 0;

  /**
   * Helper overload to provide the joint_map_builder automatically as the default joint map builder
   * TODO: Header comment.
   */
  MakeTreeResult make_tree(
    const std::vector<std::string> & joint_names,
    const std::string & base_link_name,
    const FrameDefinitions & frames)
  {
    return make_tree(joint_names, base_link_name, frames, get_joint_map_builder());
  };

  /**
   * Effectively replaces the constructor for the class, as we can only use a default constructor in plugins.
   *
   * \warning Very expensive, and obviously not real-time safe.
   * \returns True if initialization was successful. False otherwise.
   */
  bool initialize(
    const KinematicsNodeInterfaces & node_interfaces,
    KinematicsParams::SharedPtr kinematics_params = {});

  /**
   * Gets the default joint map builder used for constructing trees
   */
  [[nodiscard]] virtual const JointMapBuilder & get_joint_map_builder() const noexcept;

  /**
   * Gets the URDF model from KinematicsParams
   */
  [[nodiscard]] const urdf::Model & get_urdf_model() const;

protected:
  /**
   * Called when the kinematics plugin is created. Override this to add any set up logic for the kinematics plugin.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize() = 0;

private:
  JointMapBuilder joint_map_builder_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP
