//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_COLLISION_PLUGIN_HPP
#define ARM_KINEMATICS_COLLISION_PLUGIN_HPP

#include <arm_kinematics/forward/forward_kinematics_plugin.hpp>
#include <arm_kinematics/utilities/span.hpp>
#include <limits>
#include <rclcpp/node_interfaces/node_base_interface.hpp>
#include <rclcpp/node_interfaces/node_interfaces.hpp>
#include <rclcpp/node_interfaces/node_logging_interface.hpp>
#include <rclcpp/node_interfaces/node_parameters_interface.hpp>
#include "allowed_collision_matrix.hpp"

namespace arm_kinematics {

/**
 * Base class for plugins used to perform inverse kinematics.
 *
 * \note The responsibility of understanding where colliders are in space and how they relate to different links on the
 * robot is delegated to the caller!
 * \note The ACM can also be modified externally
 */
class CollisionPlugin {
public:
  using SharedPtr = std::shared_ptr<CollisionPlugin>;
  using CollisionNodeInterfaces =
    rclcpp::node_interfaces::NodeInterfaces<
      rclcpp::node_interfaces::NodeBaseInterface,
      rclcpp::node_interfaces::NodeLoggingInterface,
      rclcpp::node_interfaces::NodeParametersInterface>;

  /**
   * Effectively replaces the constructor for the class, as we can only use a default constructor in plugins.
   * \param node_interfaces Interfaces to use for logging, parameters, etc
   * \param collider_geometries Every collider to use in collision. Order should be preserved when calling collide()
   * \param acm The allowed collision matrix for all collider geometries.
   * \warning Very expensive, and obviously not real-time safe.
   * \returns True if initialization was successful. False otherwise.
   */
  bool initialize(
    CollisionNodeInterfaces node_interfaces,
    const std::vector<std::reference_wrapper<const urdf::Collision>> & collider_geometries,
    AllowedCollisionMatrix acm);

  void update_pose(size_t idx, const Eigen::Isometry3d & collider_pose) = 0;
  void update_poses(size_t start_idx, span<const Eigen::Isometry3d> collider_poses);

  /**
   * Perform a self intersection check with the given joint states.
   * \param collider_poses The transforms of all colliders provided in initialization
   * \returns true if there is an intersection, false if there is no intersection
   */
  bool collide() override;

  /**
   * Perform a self intersection check with the given joint states.
   * \param collider_poses The transforms of all colliders provided in initialization
   * \returns true if there is an intersection, false if there is no intersection
   */
  bool collide(
    std::vector<std::pair<size_t, size_t>> & colliding_pairs,
    size_t max_colliding_pairs) override;

  virtual bool collide() = 0;

  /**
   * Perform a self intersection check with the given joint states, preserving which colliders would intersect.
   * This overload is included for helping to build the allowed collision matrix.
   * \warning This is much more expensive than the other function overload! Use only when you need the pairs.
   * \param[in] collider_poses The transforms of all colliders provided in initialization
   * \param[out] colliding_pairs Collision pairs found in collision
   * \param max_colliding_pairs The maximum number of pairs to populate colliding pairs with.
   * \returns true if there is an intersection, false if there is no intersection
   */
  virtual bool collide(
    std::vector<std::pair<size_t, size_t>> & colliding_pairs,
    size_t max_colliding_pairs) = 0;

  /**
   * Perform a self intersection check with the given joint states, preserving which colliders would intersect.
   * This overload is included for helping to build the allowed collision matrix. Unlimited colliding pairs.
   * \warning This is much more expensive than the other function overload! Use only when you need the pairs.
   * \param[in] collider_poses The transforms of all colliders provided in initialization
   * \param[out] colliding_pairs Collision pairs found in collision
   * \returns true if there is an intersection, false if there is no intersection
   */
  bool collide(
    const span<const Eigen::Isometry3d> collider_poses,
    std::vector<std::pair<size_t, size_t>> & colliding_pairs)
  {
    // virtual functions cannot include default arguments, so this separate overload need be created.
    return collide(collider_poses, colliding_pairs, std::numeric_limits<size_t>::max());
  }

  /// Logger to use for logging
  [[nodiscard]] const rclcpp::Logger & get_logger() const noexcept;
  /// Gets interfaces from the owning ROS2 node, allowing plugins to access to parameters, logging, etc.
  [[nodiscard]] const CollisionNodeInterfaces & get_node_interfaces() const;

  /// Gets the allowed collision matrix for this collision plugin instance.
  [[nodiscard]] AllowedCollisionMatrix & get_allowed_collision_matrix() noexcept { return acm_; }
  /// Gets the allowed collision matrix for this collision plugin instance.
  [[nodiscard]] const AllowedCollisionMatrix & get_allowed_collision_matrix() const noexcept { return acm_; }

  /// Gets the number of colliders simulated, which equals the number of poses to be passed in
  [[nodiscard]] constexpr size_t size() const noexcept { return size_; }

  virtual ~CollisionPlugin() = default;

protected:
  /**
   * Called when the collision plugin is created. Override this to add any set up logic for the collision plugin.
   * \param[in] collider_geometries The shape of all colliders to model. Origin transforms will be handled externally.
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize(
    const std::vector<std::reference_wrapper<const urdf::Collision>> & collider_geometries) = 0;

private:
  /// Used to filter out collisions we don't care about (i.e. colliders on the same link or joint rotation point).
  AllowedCollisionMatrix acm_{};
  /// Logger to use for logging
  rclcpp::Logger logger_ = rclcpp::get_logger("arm_kinematics.collision");
  /// Allows us to access various things from the owning node if we need, like loggers, parameters, or in the future,
  /// maybe even topics.
  std::optional<CollisionNodeInterfaces> node_interfaces_ = std::nullopt;
  /// The number of colliders simulated, which equals the number of poses to be passed in
  size_t size_ = 0;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_COLLISION_PLUGIN_HPP
