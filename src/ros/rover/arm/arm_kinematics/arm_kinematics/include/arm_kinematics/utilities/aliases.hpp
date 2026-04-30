//
// Created by Bailey Chessum on 13/11/2025.
//
// This contains aliases for types which are just absurdly long and hard to remember.
//

#ifndef ARM_KINEMATICS_ALIASES_HPP
#define ARM_KINEMATICS_ALIASES_HPP

#include <vector>
#include <Eigen/Geometry>

namespace arm_kinematics {

using Isometry3dAllocator = Eigen::aligned_allocator<Eigen::Isometry3d>;
using Isometry3dVector = std::vector<Eigen::Isometry3d, Isometry3dAllocator>;

using Vector3dVector = std::vector<Eigen::Vector3d, Eigen::aligned_allocator<Eigen::Vector3d>>;

}

#endif //ARM_KINEMATICS_ALIASES_HPP
