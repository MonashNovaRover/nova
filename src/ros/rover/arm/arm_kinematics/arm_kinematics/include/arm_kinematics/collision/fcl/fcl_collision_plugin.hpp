//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
#define ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP

#include <arm_kinematics/collision/collision_plugin.hpp>
#include <fcl/fcl.h>
#include <arm_kinematics/collision/fcl/geometry_cache.hpp>

namespace arm_kinematics {

/**
 * FCL Callback
 */
class FclCollisionPlugin : public CollisionPlugin {
public:
  bool collide(span<const Eigen::Isometry3d> collider_poses) override;
  bool collide(
    span<const Eigen::Isometry3d> collider_poses,
    std::vector<std::pair<size_t, size_t>> & colliding_pairs,
    size_t max_colliding_pairs) override;

protected:
  bool on_initialize(const std::vector<urdf::Collision> & collider_geometries) override;

private:
  std::vector<fcl::CollisionObjectd> colliders_ = {};
  GeometryCache geometry_cache_{};

  fcl::DynamicAABBTreeCollisionManagerd manager_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
