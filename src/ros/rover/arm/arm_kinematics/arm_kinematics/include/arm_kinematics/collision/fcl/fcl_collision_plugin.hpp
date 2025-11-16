//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
#define ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP

#include <arm_kinematics/collision/collision_plugin.hpp>
#include <fcl/fcl.h>
#include <arm_kinematics/collision/fcl/geometry_cache.hpp>
#include <arm_kinematics/collision/fcl/allowed_collision_matrix.hpp>

namespace arm_kinematics {

/**
 * \brief Holds data for Colliders used in collision checks
 *
 * We are running this in the hot loop, so we want to minimize cache misses when performing collision checks.
 * Hence, we use a Struct of Arrays (SoA) approach to memory management.
 */
// class CollidersSoA {
// public:
//   using Isometry3dVector = std::vector<Eigen::Isometry3d, Eigen::aligned_allocator<Eigen::Isometry3d>>;
//
//   CollidersSoA(Isometry3dVector origin_poses, std::vector<size_t> parent_idxs, std::vector<fcl::CollisionObjectd> colliders, AllowedCollisionMatrix acm)
//       : origin_poses(std::move(origin_poses)), parent_idxs(std::move(parent_idxs)), colliders(std::move(colliders)), acm(std::move(acm))
//   {
//     for (auto& collider : colliders)
//       manager.registerObject(&collider);
//
//     manager.setup();
//   }
//
//   void set_transforms(const Isometry3dVector & link_poses) {
//     for (uint32_t i = 0; i < colliders.size(); ++i) {
//       auto& collider = colliders[i];
//       auto parent_idx = parent_idxs[i];
//       collider.setTransform(link_poses[parent_idx] * origin_poses[i]);
//     }
//
//     manager.update();
//   }
//
// private:
//   AllowedCollisionMatrix acm;
//
//   // Warning -- this must never reallocate, or the manager will throw a segfault
//
//   // Hot data
//   /// World poses per obj-id
//   //  std::vector<Eigen::Isometry3d, Eigen::aligned_allocator<Eigen::Isometry3d>> world_poses; // world pose
//
//   /// Poses local to parent link per obj-id
//   Isometry3dVector origin_poses = {};
//   std::vector<size_t> parent_idxs = {};
//
//   /// Performs the collision checks
//   fcl::DynamicAABBTreeCollisionManagerd manager{};
// };

/**
 * FCL Callback
 */

class FclCollisionPlugin : public CollisionPlugin {

  bool on_initialize(const std::vector<urdf::Collision> & collider_geometries) override;
  bool collide(span<const Eigen::Isometry3d> collider_poses) override;

private:
  AllowedCollisionMatrix acm_;
  std::vector<fcl::CollisionObjectd> colliders_ = {};
  GeometryCache geometry_cache_{};

  fcl::DynamicAABBTreeCollisionManagerd manager_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
