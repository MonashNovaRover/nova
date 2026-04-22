//
// Created by Bailey Chessum on 30/11/25.
//

#include "arm_kinematics/collision/collider_definitions.hpp"

#include <unordered_set>
#include <urdf/model.h>
#include "arm_kinematics/collision/allowed_collision_matrix.hpp"
#include "arm_kinematics/utilities/to_eigen.hpp"

namespace arm_kinematics {

ColliderDefinitions::ColliderDefinitions(const urdf::Model & urdf_model)
: ColliderDefinitions(urdf_model, {})
{
}

ColliderDefinitions::ColliderDefinitions(
  const urdf::Model & urdf_model,
  const span<const std::string> ignored_links)
{
  const std::unordered_set<std::string> ignored(ignored_links.begin(), ignored_links.end());

  size_t collider_count = 0;
  for (auto & [name, link] : urdf_model.links_) {
    if (ignored.count(name)) {
      continue;
    }
    if (const size_t collision_array_size = link->collision_array.size()) {
      collider_count += collision_array_size;
    } else if (link->collision) {
      collider_count += 1;
    }
  }

  acm = AllowedCollisionMatrix(collider_count);

  std::vector<std::string> parent_link_names{};
  Isometry3dVector origins{};

  colliders = {};
  colliders.reserve(collider_count);
  parent_link_names.reserve(collider_count);
  origins.reserve(collider_count);

  size_t i = 0;
  for (auto & [name, link] : urdf_model.links_) {
    if (ignored.count(name)) {
      continue;
    }

    if (!link->collision_array.size()) {
      if (!link->collision) {
        continue;
      }

      colliders.emplace_back(std::ref(*link->collision));
      parent_link_names.emplace_back(name);
      origins.emplace_back(to_eigen(link->collision->origin));
      ++i;
      continue;
    }

    const size_t collision_array_size = link->collision_array.size();

    for (size_t j = i; j < i + collision_array_size; ++j) {
      for (size_t k = j + 1; k < i + collision_array_size; ++k) {
        acm.set(j, k, true);
      }
    }

    for (const auto & collider : link->collision_array) {
      assert(collider);
      colliders.emplace_back(std::ref(*collider));
      parent_link_names.emplace_back(name);
      origins.emplace_back(to_eigen(collider->origin));
    }

    i += collision_array_size;
  }

  frames = FrameDefinitions(
    std::move(parent_link_names),
    std::move(origins));
}

} // arm_kinematics
