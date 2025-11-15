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
struct QueryData {
  // const CollidersSoA* S;
  const AllowedCollisionMatrix* acm;
  bool hit = false;
};

/**
 * FCL Callback to filter out allowed collisions
 */
inline bool collide_with_acm(fcl::CollisionObjectd* o1, fcl::CollisionObjectd* o2, void* ud)
{
  auto* q = static_cast<QueryData*>(ud);
  // TODO: Manage memory properly as to set the user defined data up correctly
  auto id1 = *static_cast<uint32_t*>(o1->getUserData());
  auto id2 = *static_cast<uint32_t*>(o2->getUserData());

  if (q->acm->get(id1, id2))
    return true; // skip narrow-phase

  static thread_local fcl::CollisionRequestd req = []{
    fcl::CollisionRequestd r;
    r.num_max_contacts = 1;
    r.enable_contact = false;
    r.enable_cost = false;
    r.gjk_solver_type = fcl::GJKSolverType::GST_LIBCCD;
    return r;
  }();

  fcl::CollisionResultd res;
  fcl::collide(o1, o2, req, res);

  if (res.isCollision()) {
    q->hit = true;
    return false;
  }

  return true;
}

class FclCollisionPlugin : public CollisionPlugin {

  bool on_initialize(const std::vector<urdf::Collision> & collider_geometries) override
  {
    colliders_ = {};
    for (auto urdf_geometry : collider_geometries) {
      auto geometry = geometry_cache_.from_urdf(geometry);
      colliders_.emplace_back(geometry, Eigen::Isometry3d::Identity());
    }



    // auto fk = get_fk();
    // auto & urdf_model = fk->get_urdf_model();
    //
    // // Map link name -> dense link index
    // std::unordered_map<std::string, uint32_t> link_index;
    // link_index.reserve(urdf_model.links_.size());
    // uint32_t next_link_i = 0;
    //
    // // Generate FCL collision objects for all links with colliders
    // for (const auto& [name, link] : urdf_model.links_) {
    //   // Collect all colliders for this link
    //   std::vector<std::shared_ptr<urdf::Collision>> colliders = link->collision_array;
    //   if (colliders.empty() && link->collision)
    //     colliders = {link->collision};
    //
    //   // Skip links without colliders
    //   if (colliders.empty())
    //     continue;
    //
    //   // Convert all colliders to equivalents in FCL
    //   std::vector<fcl::CollisionObjectd> fcl_colliders{};
    //   std::vector<Eigen::Isometry3d> fcl_collider_origins{};
    //   fcl_colliders.reserve(colliders.size());
    //
    //   for (const auto & collider : colliders) {
    //     if (!collider)
    //       continue;
    //
    //     const auto & col = link->collision;
    //
    //     // Create FCL geo
    //     std::shared_ptr<fcl::CollisionGeometryd> fcl_col_geo;
    //
    //     bool hasFoundCylinder = false;
    //
    //     // Convert the URDF origin to Eigen::Isometry3d
    //     Eigen::Isometry3d origin{};
    //     origin.translation() = Eigen::Vector3d(
    //       col->origin.position.x,
    //       col->origin.position.y,
    //       col->origin.position.z
    //     );
    //
    //     // URDF defines a quaternion, so we need to convert that to the eigen equivalent
    //     double rotation_data[4];
    //     col->origin.rotation.getQuaternion(rotation_data[0], rotation_data[1], rotation_data[2], rotation_data[3]);
    //     Eigen::Quaterniond rotation{rotation_data};
    //     origin.linear() = rotation.toRotationMatrix();
    //
    //     // Finally construct FCL collision object
    //     fcl::Transform3d fcl_origin(origin.matrix());
    //     fcl::CollisionObjectd obj(fcl_col_geo, fcl_origin);
    //   }
    // }
  }



  bool collide(const Isometry3dVector& collider_poses) override;

private:
  std::vector<fcl::CollisionObjectd> colliders_ = {};
  GeometryCache geometry_cache_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
