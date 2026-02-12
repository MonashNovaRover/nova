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

using Isometry3fAllocator = Eigen::aligned_allocator<Eigen::Isometry3f>;
using Isometry3fVector = std::vector<Eigen::Isometry3f, Isometry3fAllocator>;

using Vector3fVector = std::vector<Eigen::Vector3f, Eigen::aligned_allocator<Eigen::Vector3f>>;

using Twistd = Eigen::Matrix<double, 6, 1>;
using Twistf = Eigen::Matrix<float, 6, 1>;

}

#endif //ARM_KINEMATICS_ALIASES_HPP
