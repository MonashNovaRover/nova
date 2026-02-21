//
// Created by nova on 11/30/25.
//

#ifndef SCIENCE_TO_EIGEN_HPP
#define SCIENCE_TO_EIGEN_HPP

#include <Eigen/Geometry>
#include <urdf_model/pose.h>

namespace arm_kinematics {

inline Eigen::Vector3f to_eigen(const urdf::Vector3 & p)
{
  return {
    static_cast<float>(p.x),
    static_cast<float>(p.y),
    static_cast<float>(p.z)
  };
}

inline Eigen::Isometry3f to_eigen(const urdf::Pose & p)
{
  Eigen::Isometry3f T = Eigen::Isometry3f::Identity();
  T.translation() = Eigen::Vector3f(
    static_cast<float>(p.position.x),
    static_cast<float>(p.position.y),
    static_cast<float>(p.position.z));

  double x, y, z, w;
  p.rotation.getQuaternion(x, y, z, w);
  const Eigen::Quaternionf q(
    static_cast<float>(w),
    static_cast<float>(x),
    static_cast<float>(y),
    static_cast<float>(z));
  T.linear() = q.toRotationMatrix();
  return T;
}

}

#endif //SCIENCE_TO_EIGEN_HPP