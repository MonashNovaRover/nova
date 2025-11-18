//
// Created by Bailey Chessum on 12/11/2025.
//

#include <arm_kinematics/collision/fcl/geometry_cache.hpp>
#include <utility>
#include <fcl/geometry/shape/box.h>
#include <fcl/geometry/shape/capsule.h>
#include <fcl/geometry/shape/sphere.h>
#include <rclcpp/logging.hpp>

namespace arm_kinematics {

GeometryCache::GeometryCache(rclcpp::Logger logger) : logger_(std::move(logger)) {}

GeometryCache::GeoPtr GeometryCache::get_box(double x, double y, double z) {
  return std::make_shared<fcl::Boxd>(x, y, z);
}

GeometryCache::GeoPtr GeometryCache::get_sphere(double r) {
  return std::make_shared<fcl::Sphered>(r);
}

GeometryCache::GeoPtr GeometryCache::get_capsule(double r, double halfLen) {
  return std::make_shared<fcl::Capsuled>(r, halfLen);
}

GeometryCache::GeoPtr GeometryCache::from_urdf(const urdf::Collision & col, const std::string & link_name) {
  if (!col.geometry)
    return nullptr;

  switch (col.geometry->type) {
    case urdf::Geometry::BOX: {
      auto geometry = std::static_pointer_cast<urdf::Box>(col.geometry);
      return get_box(geometry->dim.x, geometry->dim.y, geometry->dim.z);
    }
    case urdf::Geometry::SPHERE: {
      auto geometry = std::static_pointer_cast<urdf::Sphere>(col.geometry);
      return get_sphere(geometry->radius);
    }
    case urdf::Geometry::CYLINDER: {
      auto geometry = std::static_pointer_cast<urdf::Cylinder>(col.geometry);
      if (warn_on_cylinder_) {
        RCLCPP_WARN(logger_, "Replacing CYLINDER with CAPSULE on link \"%s\" (conservative).",
                    link_name.c_str());
      }
      return get_capsule(geometry->radius, 0.5 * geometry->length);
    }
    default:
      RCLCPP_WARN(logger_, "Unsupported mesh geometry on link \"%s\". Skipping collider.",
                  link_name.c_str());
      return nullptr;
  }
}

} // arm_kinematics
