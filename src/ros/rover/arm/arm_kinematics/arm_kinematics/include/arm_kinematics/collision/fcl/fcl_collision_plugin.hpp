//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
#define ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP

#include <arm_kinematics/collision/collision_plugin.hpp>
#include <arm_kinematics/collision/fcl/geometry_cache.hpp>
#include <fcl/broadphase/broadphase_dynamic_AABB_tree.h>
#include <fcl/narrowphase/collision_object.h>

namespace arm_kinematics {

/**
 * Collision checking implementation using the FCL collision library
 */
class FclCollisionPlugin : public CollisionPlugin {
public:
  void update_pose(size_t idx, const Eigen::Isometry3d & collider_pose) = 0;
  void update_poses(size_t start_idx, span<const Eigen::Isometry3d> collider_poses);

  bool collide() override;
  bool collide(
    std::vector<std::pair<size_t, size_t>> & colliding_pairs,
    size_t max_colliding_pairs) override;

protected:
  bool on_initialize(const std::vector<std::reference_wrapper<const urdf::Collision>> & collider_geometries) override;

private:
  /// The collider geometries used in collision checking. DO NOT ADD/REMOVE ELEMENTS OUTSIDE on_initialize()
  std::vector<fcl::CollisionObjectd> colliders_{};
  GeometryCache geometry_cache_{};
  fcl::DynamicAABBTreeCollisionManagerd manager_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
