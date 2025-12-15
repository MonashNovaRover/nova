//
// Created by Bailey Chessum on 15/10/2025.
//

#include "arm_kinematics/collision/fcl/fcl_collision_plugin.hpp"

#include <fcl/fcl.h>

namespace arm_kinematics {

/**
 * Struct used in FCL callback
 */
struct QueryData {
  const AllowedCollisionMatrix & acm;
  bool hit = false;
};

/**
 * FCL Callback to filter out allowed collisions
 *
 * \warning indices to lookup in the ACM must be stored as the pointer itself for the collider user data fields.
 * Reinterpret cast to std::uintptr_t.
 */
inline bool collide_with_acm(fcl::CollisionObjectf* o1, fcl::CollisionObjectf* o2, void* ud)
{
  auto* q = static_cast<QueryData*>(ud);
  const auto id1 = static_cast<size_t>(reinterpret_cast<std::uintptr_t>(o1->getUserData()));
  const auto id2 = static_cast<size_t>(reinterpret_cast<std::uintptr_t>(o2->getUserData()));

  if (q->acm.get(id1, id2))
    return true;  //< Skip narrow-phase, continue collision checks

  static thread_local fcl::CollisionRequestf req = []{
    fcl::CollisionRequestf r;
    r.num_max_contacts = 1;
    r.enable_contact = false;
    r.enable_cost = false;
    r.gjk_solver_type = fcl::GJKSolverType::GST_LIBCCD;
    return r;
  }();

  fcl::CollisionResultf res;
  fcl::collide(o1, o2, req, res);

  if (res.isCollision()) {
    q->hit = true;
    return false;   //< Stop collision checks
  }

  return true;  //< Continue collision checks
}

bool FclCollisionPlugin::collide() {
  QueryData query {
    get_allowed_collision_matrix()
  };

  manager_.update();
  manager_.collide(&query, collide_with_acm);

  return query.hit;
}

/**
 * Struct used in FCL callback with pairs
 */
struct QueryDataWithPairs : QueryData {
  std::vector<std::pair<size_t, size_t>> & pairs;
  const size_t max_pairs{};
};

/**
 * FCL Callback to filter out allowed collisions
 *
 * \warning indices to lookup in the ACM must be stored as the pointer itself for the collider user data fields.
 * Reinterpret cast to std::uintptr_t.
 */
inline bool collide_with_acm_and_pairs(fcl::CollisionObjectf* o1, fcl::CollisionObjectf* o2, void* ud)
{
  auto* q = static_cast<QueryDataWithPairs*>(ud);
  const auto id1 = static_cast<size_t>(reinterpret_cast<std::uintptr_t>(o1->getUserData()));
  const auto id2 = static_cast<size_t>(reinterpret_cast<std::uintptr_t>(o2->getUserData()));

  if (q->acm.get(id1, id2))
    return true;  //< Skip narrow-phase, continue collision checks

  // Collision narrow phase
  static thread_local fcl::CollisionRequestf req = []{
    fcl::CollisionRequestf r;
    r.num_max_contacts = 1;
    r.enable_contact = false;
    r.enable_cost = false;
    r.gjk_solver_type = fcl::GJKSolverType::GST_LIBCCD;
    return r;
  }();

  fcl::CollisionResultf res;
  fcl::collide(o1, o2, req, res);

  if (res.isCollision()) {
    q->hit = true;
    q->pairs.emplace_back(id1, id2);

    if (q->pairs.size() >= q->max_pairs)
      return false;   //< Stop collision checks
  }

  return true;  //< Continue collision checks
}

bool FclCollisionPlugin::collide(
  std::vector<std::pair<size_t, size_t>> & colliding_pairs,
  const size_t max_colliding_pairs)
{
  // Ensure collisions output is reset
  colliding_pairs.clear();

  QueryDataWithPairs query {
    get_allowed_collision_matrix(),
    false,
    colliding_pairs,
    max_colliding_pairs
  };

  manager_.update();
  manager_.collide(&query, collide_with_acm);

  return query.hit;
}

void FclCollisionPlugin::update_pose(const size_t idx, const Eigen::Isometry3f& collider_pose) {
  colliders_[idx].setTransform(collider_pose);
}

void FclCollisionPlugin::update_poses(const size_t start_idx, const span<const Eigen::Isometry3f> collider_poses) {
  // Copy poses into the colliders
  const size_t end = start_idx + collider_poses.size();
  auto pose_it = collider_poses.begin();
  for (size_t i = start_idx; i < end; ++i) {
    colliders_[i].setTransform(*pose_it);
    ++pose_it;
  }
}

bool FclCollisionPlugin::on_initialize(
  const std::vector<std::reference_wrapper<const urdf::Collision>> & collider_geometries)
{
  geometry_cache_ = {};
  colliders_ = {};
  colliders_.reserve(collider_geometries.size());

  for (size_t i = 0; i < collider_geometries.size(); ++i) {
    std::shared_ptr geometry = geometry_cache_.from_urdf(collider_geometries[i]);
    auto& collider = colliders_.emplace_back(geometry, Eigen::Isometry3f::Identity());

    collider.setUserData(reinterpret_cast<void*>(static_cast<std::uintptr_t>(i)));
  }

  manager_.clear();
  for (auto& collider : colliders_) {
    manager_.registerObject(&collider);
  }
  // TODO: Do we need a reasonable initial pose for setup?
  manager_.setup();

  return true;
}

} // arm_kinematics
