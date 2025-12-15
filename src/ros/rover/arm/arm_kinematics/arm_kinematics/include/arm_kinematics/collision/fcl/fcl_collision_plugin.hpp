//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
#define ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP

#include <fcl/broadphase/broadphase_dynamic_AABB_tree.h>
#include <fcl/narrowphase/collision_object.h>

#include "arm_kinematics/visibility_control.h"
#include "arm_kinematics/collision/discrete_collision_plugin.hpp"
#include "arm_kinematics/collision/fcl/geometry_cache.hpp"

namespace arm_kinematics {

/**
 * Collision checking implementation using the FCL collision library.
 */
class ARM_KINEMATICS_PUBLIC FclCollisionPlugin : public DiscreteCollisionPlugin {
public:
  void update_pose(size_t idx, const Eigen::Isometry3f & collider_pose) override;
  void update_poses(size_t start_idx, span<const Eigen::Isometry3f> collider_poses) override;

  bool collide() override;
  bool collide(
    std::vector<std::pair<size_t, size_t>> & colliding_pairs,
    size_t max_colliding_pairs) override;

protected:
  bool on_initialize(const std::vector<std::reference_wrapper<const urdf::Collision>> & collider_geometries) override;

private:
  /// The collider geometries used in collision checking. DO NOT ADD/REMOVE ELEMENTS OUTSIDE on_initialize()
  std::vector<fcl::CollisionObjectf> colliders_{};
  GeometryCache geometry_cache_{};
  fcl::DynamicAABBTreeCollisionManagerf manager_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
