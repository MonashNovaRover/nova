//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
#define ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP

#include <arm_kinematics/collision/collision_plugin.hpp>
#include <fcl/fcl.h>

namespace arm_kinematics {

class FclCollisionPlugin : public CollisionPlugin {

  bool on_initialize() override {
    auto fk = get_fk();
    auto & urdf_model = fk->get_urdf_model();


    // Generate FCL collision objects for all links with colliders
    for (const auto& [name, link] : urdf_model.links_) {
      // Get all colliders for this link
      std::vector<std::shared_ptr<urdf::Collision>> colliders = link->collision_array;
      if (colliders.empty() && link->collision)
        colliders = {link->collision};

      // Skip links without colliders
      if (colliders.empty())
        continue;

      // Convert all colliders to equivalents in FCL
      std::vector<fcl::CollisionObjectd> fcl_colliders{};
      std::vector<Eigen::Isometry3d> fcl_collider_origins{};
      fcl_colliders.reserve(colliders.size());

      for (const auto & collider : colliders) {
        if (!collider)
          continue;

        const auto & col = link->collision;

        // Create FCL geo
        std::shared_ptr<fcl::CollisionGeometryd> fcl_col_geo;

        if (col->geometry->type == urdf::Geometry::BOX) {
          auto box = std::dynamic_pointer_cast<urdf::Box>(col->geometry);
          if (!box) continue;
          fcl_col_geo = std::make_shared<fcl::Boxd>(box->dim.x, box->dim.y, box->dim.z);
        }
        else if (col->geometry->type == urdf::Geometry::SPHERE) {
          auto sphere = std::dynamic_pointer_cast<urdf::Sphere>(col->geometry);
          if (!sphere) continue;
          fcl_col_geo = std::make_shared<fcl::Sphered>(sphere->radius);
        }
        else if (col->geometry->type == urdf::Geometry::CYLINDER) {
          auto cylinder = std::dynamic_pointer_cast<urdf::Cylinder>(col->geometry);
          if (!cylinder) continue;
          fcl_col_geo = std::make_shared<fcl::Cylinderd>(cylinder->radius, cylinder->length);
        }
        else if (col->geometry->type == urdf::Geometry::MESH) {
          auto mesh = std::dynamic_pointer_cast<urdf::Mesh>(col->geometry);
          if (!mesh) continue;
          // We could support meshes, but I'm not sure we should
          RCLCPP_WARN(get_logger(), "Unable to model collision for link \"%s\" -- FclCollisionPlugin does not support "
                                    "mesh geometries in collisions.", link->name.c_str());
        }



        // Convert the URDF origin to Eigen::Isometry3d
        Eigen::Isometry3d origin{};
        origin.translation() = Eigen::Vector3d(
          col->origin.position.x,
          col->origin.position.y,
          col->origin.position.z
        );

        // URDF defines a quaternion, so we need to convert that to the eigen equivalent
        double rotation_data[4];
        col->origin.rotation.getQuaternion(rotation_data[0], rotation_data[1], rotation_data[2], rotation_data[3]);
        Eigen::Quaterniond rotation{rotation_data};
        origin.linear() = rotation.toRotationMatrix();

        // Finally construct FCL collision object
        fcl::Transform3d fcl_origin(origin.matrix());
        fcl::CollisionObjectd obj(fcl_col_geo, fcl_origin);
      }
    }




  }


};

} // arm_kinematics

#endif //ARM_KINEMATICS_FCL_COLLISION_PLUGIN_HPP
