//
// Created by Bailey Chessum on 1/05/2026.
//

#ifndef ARM_KINEMATICS_TWIST_HPP
#define ARM_KINEMATICS_TWIST_HPP

#include <cassert>

#include <Eigen/Geometry>

namespace arm_kinematics {

/**
 * Strong 6D twist type.
 *
 * This corresponds to a spatial motion vector in rigid-body dynamics literature, but preserves
 * ROS2's existing coefficient ordering for interoperability:
 *   - segment<3>(0) is linear velocity
 *   - segment<3>(3) is angular velocity
 *
 * When transformed by an Eigen isometry `T_ab`, `T_ab * twist_b` yields the same physical twist
 * expressed in frame `a`, where `T_ab` maps coordinates from frame `b` into frame `a`.
 */
template<typename Scalar>
class Twist : public Eigen::Matrix<Scalar, 6, 1> {
public:
  using Base = Eigen::Matrix<Scalar, 6, 1>;
  using Vector3 = Eigen::Matrix<Scalar, 3, 1>;
  using Isometry3 = Eigen::Transform<Scalar, 3, Eigen::Isometry>;

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  using Base::Base;
  using Base::operator=;

  Twist() = default;
  Twist(const Twist &) = default;
  Twist(Twist &&) noexcept = default;
  Twist & operator=(const Twist &) = default;
  Twist & operator=(Twist &&) noexcept = default;

  Twist(const Base & other)
  : Base(other)
  {
  }

  Twist(Vector3 linear, Vector3 angular)
  {
    this->linear() = std::move(linear);
    this->angular() = std::move(angular);
  }

  template<typename Derived>
  Twist(const Eigen::MatrixBase<Derived> & other)
  : Base(other)
  {
    EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived);
    assert(other.rows() == 6 && "Twist expects a 6x1 Eigen expression");
  }

  template<typename Derived>
  Twist & operator=(const Eigen::MatrixBase<Derived> & other)
  {
    EIGEN_STATIC_ASSERT_VECTOR_ONLY(Derived);
    assert(other.rows() == 6 && "Twist expects a 6x1 Eigen expression");
    Base::operator=(other);
    return *this;
  }

  [[nodiscard]] Base & vector() noexcept { return *this; }
  [[nodiscard]] const Base & vector() const noexcept { return *this; }

  [[nodiscard]] Eigen::VectorBlock<Base, 3> linear() noexcept
  {
    return this->template segment<3>(0);
  }

  [[nodiscard]] Eigen::VectorBlock<const Base, 3> linear() const noexcept
  {
    return this->template segment<3>(0);
  }

  [[nodiscard]] Eigen::VectorBlock<Base, 3> angular() noexcept
  {
    return this->template segment<3>(3);
  }

  [[nodiscard]] Eigen::VectorBlock<const Base, 3> angular() const noexcept
  {
    return this->template segment<3>(3);
  }
};

template<typename Scalar>
[[nodiscard]] Twist<Scalar> operator*(
  const typename Twist<Scalar>::Isometry3 & transform,
  const Twist<Scalar> & twist)
{
  Twist<Scalar> result(Twist<Scalar>::Zero());
  const auto transformed_angular = transform.linear() * twist.angular();
  result.linear().noalias() = transform.linear() * twist.linear();
  result.linear() += transform.translation().cross(transformed_angular);
  result.angular() = transformed_angular;
  return result;
}

/**
 * Motion cross product with this package's `[linear; angular]` twist ordering.
 */
template<typename Scalar>
[[nodiscard]] Twist<Scalar> cross_motion(const Twist<Scalar> & lhs, const Twist<Scalar> & rhs)
{
  Twist<Scalar> result(Twist<Scalar>::Zero());
  result.linear() = lhs.angular().cross(rhs.linear()) + lhs.linear().cross(rhs.angular());
  result.angular() = lhs.angular().cross(rhs.angular());
  return result;
}

using Twistd = Twist<double>;
using Twistf = Twist<float>;

} // namespace arm_kinematics

#endif // ARM_KINEMATICS_TWIST_HPP
