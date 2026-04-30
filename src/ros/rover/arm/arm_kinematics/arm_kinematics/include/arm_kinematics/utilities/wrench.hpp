//
// Created by Bailey Chessum on 1/05/2026.
//

#ifndef ARM_KINEMATICS_WRENCH_HPP
#define ARM_KINEMATICS_WRENCH_HPP

#include <cassert>

#include <Eigen/Geometry>

#include "arm_kinematics/utilities/twist.hpp"

namespace arm_kinematics {

/**
 * Strong 6D wrench type.
 *
 * This corresponds to a spatial force vector in rigid-body dynamics literature, but preserves
 * a positional pairing with ROS2's twist ordering:
 *   - segment<3>(0) is force
 *   - segment<3>(3) is moment
 *
 * When transformed by an Eigen isometry `T_ab`, `T_ab * wrench_b` yields the same physical wrench
 * expressed in frame `a`, where `T_ab` maps coordinates from frame `b` into frame `a`.
 */
template<typename Scalar>
class Wrench : public Eigen::Matrix<Scalar, 6, 1> {
public:
  using Base = Eigen::Matrix<Scalar, 6, 1>;
  using Vector3 = Eigen::Matrix<Scalar, 3, 1>;
  using Isometry3 = Eigen::Transform<Scalar, 3, Eigen::Isometry>;

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  using Base::Base;
  using Base::operator=;

  Wrench() = default;
  Wrench(const Wrench &) = default;
  Wrench(Wrench &&) noexcept = default;
  Wrench & operator=(const Wrench &) = default;
  Wrench & operator=(Wrench &&) noexcept = default;

  Wrench(const Base & other)
  : Base(other)
  {
  }

  Wrench(Vector3 force, Vector3 moment)
  {
    this->force() = std::move(force);
    this->moment() = std::move(moment);
  }

  template<typename Derived>
  Wrench(const Eigen::MatrixBase<Derived> & other)
  : Base(other)
  {
    EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived);
    assert(other.rows() == 6 && "Wrench expects a 6x1 Eigen expression");
  }

  template<typename Derived>
  Wrench & operator=(const Eigen::MatrixBase<Derived> & other)
  {
    EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived);
    assert(other.rows() == 6 && "Wrench expects a 6x1 Eigen expression");
    Base::operator=(other);
    return *this;
  }

  [[nodiscard]] Base & vector() noexcept { return *this; }
  [[nodiscard]] const Base & vector() const noexcept { return *this; }

  [[nodiscard]] Eigen::VectorBlock<Base, 3> force() noexcept
  {
    return this->template segment<3>(0);
  }

  [[nodiscard]] Eigen::VectorBlock<const Base, 3> force() const noexcept
  {
    return this->template segment<3>(0);
  }

  [[nodiscard]] Eigen::VectorBlock<Base, 3> moment() noexcept
  {
    return this->template segment<3>(3);
  }

  [[nodiscard]] Eigen::VectorBlock<const Base, 3> moment() const noexcept
  {
    return this->template segment<3>(3);
  }
};

template<typename Scalar>
[[nodiscard]] Wrench<Scalar> operator*(
  const typename Wrench<Scalar>::Isometry3 & transform,
  const Wrench<Scalar> & wrench)
{
  Wrench<Scalar> result(Wrench<Scalar>::Zero());
  const auto transformed_force = transform.linear() * wrench.force();
  result.force() = transformed_force;
  result.moment().noalias() = transform.linear() * wrench.moment();
  result.moment() += transform.translation().cross(transformed_force);
  return result;
}

/**
 * Force cross product with this package's `[linear; angular]` twist ordering and
 * `[force; moment]` wrench ordering.
 */
template<typename Scalar>
[[nodiscard]] Wrench<Scalar> cross_force(const Twist<Scalar> & twist, const Wrench<Scalar> & wrench)
{
  Wrench<Scalar> result(Wrench<Scalar>::Zero());
  result.force() = twist.angular().cross(wrench.force());
  result.moment() = twist.angular().cross(wrench.moment()) + twist.linear().cross(wrench.force());
  return result;
}

template<typename Scalar>
[[nodiscard]] Scalar power(const Wrench<Scalar> & wrench, const Twist<Scalar> & twist)
{
  return wrench.force().dot(twist.linear()) + wrench.moment().dot(twist.angular());
}

using Wrenchd = Wrench<double>;
using Wrenchf = Wrench<float>;

/**
 * Semantic alias for 6D linear/angular momentum values.
 *
 * This remains storage-compatible with `Wrench`, but gives call sites a clearer name when a
 * force-shaped 6D quantity represents momentum rather than an applied wrench.
 */
template<typename Scalar>
using Momentum = Wrench<Scalar>;

using Momentumd = Momentum<double>;
using Momentumf = Momentum<float>;

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_WRENCH_HPP
