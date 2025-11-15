//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_COLLISION_PLUGIN_HPP
#define ARM_KINEMATICS_COLLISION_PLUGIN_HPP

#include <arm_kinematics/kinematics_plugins/forward_kinematics_plugin.hpp>
#include <arm_kinematics/aliases.hpp>
#include <arm_kinematics/span.hpp>

namespace arm_kinematics {

/**
 * Base class for plugins used to perform inverse kinematics.
 *
 * \note The responsibility of understanding where colliders are in space and how they relate to different links on the
 * robot is delegated to the caller!
 */
class CollisionPlugin {
public:
  using CollisionNodeInterfaces =
    rclcpp::node_interfaces::NodeInterfaces<
      rclcpp::node_interfaces::NodeBaseInterface,
      rclcpp::node_interfaces::NodeLoggingInterface,
      rclcpp::node_interfaces::NodeParametersInterface>;

  /**
   * Effectively replaces the constructor for the class, as we can only use a default constructor in plugins.
   *
   * \warning Very expensive, and obviously not real-time safe.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  bool initialize(
    KinematicsBase::KinematicsNodeInterfaces node_interfaces,
    const FrameDefinitions & collider_frames,
    const std::vector<urdf::Collision> & collider_geometries);

  /**
   * Perform a self intersection check with the given joint states.
   *
   * \param joint_states Positions of all joints, as provided by your joint map
   * \param chain The FK plugin chain that maps joint_states to
   *
   * \returns true if there is an intersection, false if there is no intersection
   */
  virtual bool collide(span<const Eigen::Isometry3d> collider_poses) = 0;

  /**
   * Perform a self intersection check with the given joint states.
   *
   * \param joint_states Positions of all joints, as provided by your joint map
   * \param chain The FK plugin chain that maps joint_states to
   *
   * \returns true if there is an intersection, false if there is no intersection
   */
  bool collide(const Isometry3dVector & collider_poses) {
    return collide({collider_poses.data(), collider_poses.size()});
  }

  /// Gets the ForwardKinematicsPlugin used to position colliders correctly in a common reference frame
  [[nodiscard]] const ForwardKinematicsPlugin::SharedPtr & get_fk() const noexcept;

  /// Logger to use for logging
  [[nodiscard]] const rclcpp::Logger & get_logger() const noexcept;
  /// Gets interfaces from the owning ROS2 node, allowing plugins to access to parameters, logging, etc.
  [[nodiscard]] const CollisionNodeInterfaces & get_node_interfaces() const;

protected:
  /**
   * Called when the collision plugin is created. Override this to add any set up logic for the collision plugin.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize(const std::vector<urdf::Collision> & collider_geometries) = 0;

private:
  ForwardKinematicsPlugin::SharedPtr fk_ = nullptr;

  /// Logger to use for logging
  rclcpp::Logger logger_ = rclcpp::get_logger("arm_kinematics.collision");
  /// Allows us to access various things from the owning node if we need, like loggers, parameters, or in the future,
  /// maybe even topics.
  std::optional<CollisionNodeInterfaces> node_interfaces_ = std::nullopt;

};

} // arm_kinematics

#endif //ARM_KINEMATICS_COLLISION_PLUGIN_HPP
