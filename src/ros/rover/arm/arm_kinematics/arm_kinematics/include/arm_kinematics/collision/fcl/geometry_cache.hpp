//
// Created by Bailey Chessum on 12/11/2025.
//

#ifndef ARM_KINEMATICS_GEOMETRY_CACHE_HPP
#define ARM_KINEMATICS_GEOMETRY_CACHE_HPP

#include <memory>
#include <fcl/fcl.h>
#include <urdf/model.h>
#include <rclcpp/logger.hpp>

namespace arm_kinematics {

/**
 * Shares identical FCL primitives across many CollisionObjects.
 * TODO: I currently just construct the colliders every time. Add caching
 */
class GeometryCache {
public:
  using GeoPtr = std::shared_ptr<const fcl::CollisionGeometryd>;

  explicit GeometryCache(rclcpp::Logger logger);

  static GeoPtr get_box(double x, double y, double z);
  static GeoPtr get_sphere(double r);
  static GeoPtr get_capsule(double r, double halfLen);

  /**
   * Helper to convert URDF collision geometries to types used in FCL for collision.
   * \note Treats cylinders as capsules (conservatively).
   * \param col the collider from the URDF
   * \param link_name the link name the collider belongs to, used in logging warning messages.
   */
  GeoPtr from_urdf(const urdf::Collision& col, const std::string& link_name);

private:
  rclcpp::Logger logger_ = rclcpp::get_logger("geometry_cache");
  bool warn_on_cylinder_ = true;
};

} // arm_kinematics

#endif //ARM_KINEMATICS_GEOMETRY_CACHE_HPP
