//
// Created by Bailey Chessum on 12/11/2025.
//

#include "arm_kinematics/plugins/collision/fcl/geometry_cache.hpp"

#include <utility>
#include <fcl/geometry/shape/box.h>
#include <fcl/geometry/shape/cylinder.h>
#include <fcl/geometry/shape/sphere.h>
#include <rclcpp/logging.hpp>

namespace arm_kinematics {

GeometryCache::GeometryCache(rclcpp::Logger logger) : logger_(std::move(logger)) {}

GeometryCache::GeoPtr GeometryCache::get_box(const double x, const double y, const double z) {
  return std::make_unique<fcl::Boxd>(x, y, z);
}

GeometryCache::GeoPtr GeometryCache::get_sphere(const double r) {
  return std::make_unique<fcl::Sphered>(r);
}

GeometryCache::GeoPtr GeometryCache::get_cylinder(const double r, const double length) {
  return std::make_unique<fcl::Cylinderd>(r, length);
}

bool GeometryCache::supports_geometry(const urdf::Collision & col) noexcept
{
  if (!col.geometry) {
    return false;
  }

  switch (col.geometry->type) {
    case urdf::Geometry::BOX:
    case urdf::Geometry::SPHERE:
    case urdf::Geometry::CYLINDER:
      return true;
    default:
      return false;
  }
}

GeometryCache::GeoPtr GeometryCache::from_urdf(const urdf::Collision & col, const std::string & link_name) {
  if (!col.geometry)
    return nullptr;

  switch (col.geometry->type) {
    case urdf::Geometry::BOX: {
      const auto geometry = std::static_pointer_cast<urdf::Box>(col.geometry);
      return get_box(geometry->dim.x, geometry->dim.y, geometry->dim.z);
    }
    case urdf::Geometry::SPHERE: {
      const auto geometry = std::static_pointer_cast<urdf::Sphere>(col.geometry);
      return get_sphere(geometry->radius);
    }
    case urdf::Geometry::CYLINDER: {
      const auto geometry = std::static_pointer_cast<urdf::Cylinder>(col.geometry);
      return get_cylinder(geometry->radius, geometry->length);
    }
    default:
      RCLCPP_WARN(logger_, "Unsupported geometry on link \"%s\". Skipping collider.",
                  link_name.c_str());
      return nullptr;
  }
}

} // arm_kinematics
