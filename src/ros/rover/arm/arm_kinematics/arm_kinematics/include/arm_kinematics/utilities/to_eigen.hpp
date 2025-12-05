//
// Created by nova on 11/30/25.
//

#ifndef SCIENCE_TO_EIGEN_HPP
#define SCIENCE_TO_EIGEN_HPP

#include <Eigen/Geometry>
#include <urdf_model/pose.h>

namespace arm_kinematics {

inline Eigen::Vector3d to_eigen(const urdf::Vector3 & p)
{
  return {p.x, p.y, p.z};
}

inline Eigen::Isometry3d to_eigen(const urdf::Pose & p)
{
  Eigen::Isometry3d T = Eigen::Isometry3d::Identity();
  T.translation() = Eigen::Vector3d(p.position.x, p.position.y, p.position.z);

  double x, y, z, w;
  p.rotation.getQuaternion(x, y, z, w);
  const Eigen::Quaterniond q(w, x, y, z);
  T.linear() = q.toRotationMatrix();
  return T;
}

}

#endif //SCIENCE_TO_EIGEN_HPP