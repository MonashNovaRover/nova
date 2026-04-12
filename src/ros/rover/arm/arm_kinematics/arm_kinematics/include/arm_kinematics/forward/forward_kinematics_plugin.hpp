//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP
#define ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP

#include <arm_kinematics/visibility_control.h>
#include <optional>
#include <string>
#include <vector>
#include <urdf/model.h>
#include <arm_kinematics/common/kinematics_base.hpp>
#include <arm_kinematics/joint_map/joint_map_builder.hpp>
#include <arm_kinematics/joint_map/state_interface_definition.hpp>
#include <arm_kinematics/joint_map/transmission_analysis.hpp>
#include <arm_kinematics/joint_map/transmission_types.hpp>
#include <arm_kinematics/utilities/aliases.hpp>
#include <arm_kinematics/utilities/expected.hpp>
#include <arm_kinematics/utilities/span.hpp>
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
    using UniquePtr = std::unique_ptr<Tree>;

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
   * Error returned by `make_tree` when tree construction cannot complete. Tree construction
   * has several distinct failure modes — joint-map construction failure, frame-tree
   * construction failure, name-resolution failure for the convenience overload — and squashing
   * them all into one type would lose actionable detail. The `kind` discriminator says which
   * failure occurred; `joint_map_error` is populated when the underlying cause is a structured
   * `JointMapBuildError` (kinds `JointMapBuildFailed` and `UnknownJoint`).
   */
  struct MakeTreeError {
    enum class Kind {
      /// The joint map builder rejected the request. `joint_map_error` carries the structured
      /// reason (missing inputs, ambiguity, unknown joint from the builder's perspective).
      JointMapBuildFailed,
      /// `AnalysisTree::make_compute_frame_tree()` failed (e.g. unresolved frame parent, bad
      /// link reference). The error string from the analysis tree lives in `message`.
      FrameTreeFailed,
      /// One or more joint names in the request could not be resolved against the FK plugin's
      /// analysis (joint name not in URDF). `joint_map_error` is populated with the equivalent
      /// `JointMapBuildError::Kind::UnknownJoint` slice.
      UnknownJoint,
    };
    Kind kind = Kind::JointMapBuildFailed;
    std::string message{};
    /// Populated when `kind` is `JointMapBuildFailed` or `UnknownJoint`. Empty for
    /// `FrameTreeFailed`.
    std::optional<JointMapBuildError> joint_map_error{};
  };

  /**
   * When you create a Tree, the order in which you provide the requested frames might need to change. So, I can't just
   * provide you with the tree that you created, but also with information about how you might need to rearrange your
   * data to match the input order it needs. Hence, this struct exists. Sorry!
   *
   * `make_tree` returns `tl::expected<MakeTreeResult, MakeTreeError>`. On success, structured
   * bindings work directly on `value()`:
   * \code
   *   auto result = plugin->make_tree({"j1", "j2"}, "base_link", {"ee_link"});
   *   if (!result) { handle(result.error()); return; }
   *   auto & [tree, order] = result.value();
   * \endcode
   */
  struct MakeTreeResult {
    /**
     * The FK tree pointer that you wanted to make.
     */
    Tree::UniquePtr tree;

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
    Order<> frame_order;
  };

  /**
   * \brief Constructs the plugin implementation's Tree subclass — the canonical (fast-path)
   * overload taking already-resolved `StateInterfaceId`s.
   * \see ForwardKinematicsPlugin::Tree
   *
   * \param input_state_interfaces[in] The state interfaces the FK tree will receive at runtime
   *   in `position_fk(joint_states, ...)`. Each id must be valid against the FK plugin's
   *   transmission analysis (see `get_transmission_analysis()`).
   * \param base_link_name[in] The name of the frame to act as the origin
   * \param frames[in] The names and offsets from links to calculate Eigen::Isometry3d values
   *   for in `Tree::position_fk()`.
   * \param joint_map_builder[in] The builder used to construct the joint map needed for the
   *   tree. Must already have its analysis populated with all the joints/state interfaces the
   *   FK tree references.
   * \returns A `tl::expected` wrapping the tree on success, or a `MakeTreeError` on failure.
   *   Failure modes include joint-map construction failure (`JointMapBuildFailed` — wraps the
   *   underlying `JointMapBuildError`) and frame-tree construction failure (`FrameTreeFailed`).
   *
   * \warning Likely expensive, and obviously not real-time safe.
   * \note From testing, takes < .1 millisecond on a simple URDF.
   * \warning The parent `ForwardKinematicsPlugin` must stay alive for the lifetime of any
   *   trees it produces; tree implementations may reference memory from the parent.
   */
  virtual tl::expected<MakeTreeResult, MakeTreeError> make_tree(
    span<const StateInterfaceDefinition> input_state_interfaces,
    const std::string & base_link_name,
    const FrameDefinitions & frames,
    const JointMapBuilder & joint_map_builder) = 0;

  /**
   * \brief Convenience overload that accepts `NamedStateInterfaceDefinition`s and resolves
   * them to `StateInterfaceDefinition`s via joint name lookup against the FK plugin's analysis
   * before delegating to the main overload above.
   *
   * Default implementation looks up each joint name in `get_transmission_analysis()`. If any
   * joint name is unknown, returns a `MakeTreeError` with `Kind::UnknownJoint` (the wrapped
   * `joint_map_error` carries a `JointMapBuildError::Kind::UnknownJoint` slice with the full
   * list of unresolvable entries).
   *
   * Subclasses can override if they want a different resolution policy.
   *
   * The main `StateInterfaceDefinition`-based overload is the canonical fast path; this overload
   * exists for callers that have joint names + interface ids on hand and don't want to do the
   * resolution themselves.
   */
  virtual tl::expected<MakeTreeResult, MakeTreeError> make_tree(
    span<const NamedStateInterfaceDefinition> named_input_state_interfaces,
    const std::string & base_link_name,
    const FrameDefinitions & frames,
    const JointMapBuilder & joint_map_builder);

  /// Helper overload — defaults the builder to `get_joint_map_builder()`.
  tl::expected<MakeTreeResult, MakeTreeError> make_tree(
    span<const StateInterfaceDefinition> input_state_interfaces,
    const std::string & base_link_name,
    const FrameDefinitions & frames)
  {
    return make_tree(input_state_interfaces, base_link_name, frames, get_joint_map_builder());
  }

  /// Helper overload — defaults the builder to `get_joint_map_builder()`. Named convenience.
  tl::expected<MakeTreeResult, MakeTreeError> make_tree(
    span<const NamedStateInterfaceDefinition> named_input_state_interfaces,
    const std::string & base_link_name,
    const FrameDefinitions & frames)
  {
    return make_tree(
      named_input_state_interfaces, base_link_name, frames, get_joint_map_builder());
  }

  /**
   * Effectively replaces the constructor for the class, as we can only use a default constructor in plugins.
   *
   * \warning Very expensive, and obviously not real-time safe.
   * \returns True if initialization was successful. False otherwise.
   */
  bool initialize(
    const KinematicsNodeInterfaces & node_interfaces,
    const RobotModel & robot_model,
    KinematicsParams::SharedPtr kinematics_params);

  /**
   * \brief Gets the transmission analysis used by this FK plugin.
   *
   * Different FK plugin implementations may choose to provide a transmission analysis that
   * differs from the shared default analysis exposed by RobotModel. The returned analysis
   * is logically frozen — callers must not assume it can be mutated.
   *
   * The default implementation delegates to `RobotModel::get_default_transmission_analysis()`.
   *
   * \warning Subclasses that cache or wrap a `JointMapBuilder` against this analysis are
   * responsible for documenting their own pointer-stability requirements. The base class makes
   * no assumptions either way.
   */
  [[nodiscard]] virtual const TransmissionAnalysis & get_transmission_analysis() const noexcept;

  /**
   * \brief Gets the joint map builder used by `make_tree()` convenience overloads.
   *
   * The base class declares this as pure virtual: each concrete plugin chooses (and owns) its
   * own joint map builder, so the abstract base does not pull in any concrete builder type.
   * Concrete plugins typically lazily construct a builder against their
   * `get_transmission_analysis()`.
   */
  [[nodiscard]] virtual const JointMapBuilder & get_joint_map_builder() const noexcept = 0;

protected:
  /**
   * Called when the kinematics plugin is created. Override this to add any set up logic for the kinematics plugin.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize() = 0;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_FORWARD_KINEMATICS_PLUGIN_HPP
