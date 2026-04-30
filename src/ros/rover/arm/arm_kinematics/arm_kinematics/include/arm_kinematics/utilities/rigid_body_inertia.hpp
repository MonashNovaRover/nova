//
// Created by Bailey Chessum on 2/05/2026.
//

#ifndef ARM_KINEMATICS_RIGID_BODY_INERTIA_HPP
#define ARM_KINEMATICS_RIGID_BODY_INERTIA_HPP

#include <cassert>

#include <Eigen/Geometry>

#include "arm_kinematics/utilities/wrench.hpp"

namespace arm_kinematics {

namespace detail {
  template<typename Scalar>
  [[nodiscard]] Eigen::Matrix<Scalar, 3, 3> skew_symmetric(
    const Eigen::Matrix<Scalar, 3, 1> & vector)
  {
    Eigen::Matrix<Scalar, 3, 3> skew;
    skew << Scalar(0), -vector.z(), vector.y(),
      vector.z(), Scalar(0), -vector.x(),
      -vector.y(), vector.x(), Scalar(0);
    return skew;
  }

  template<typename Scalar>
  [[nodiscard]] bool is_zero(const Scalar value)
  {
    return value == Scalar(0);
  }
}  // namespace detail

/**
 * Strong rigid-body inertia type.
 *
 * This corresponds to a rigid-body spatial inertia representation in rigid-body dynamics
 * literature, but it does not store a frame identity. Its numeric contents are meaningful only
 * relative to caller-supplied external context.
 *
 * The stored representation is origin-based:
 *   - `mass`
 *   - `first_moment = mass * com`
 *   - rotational inertia about the current expression origin
 *
 * CoM-oriented quantities are derived on demand from that stored form.
 */
template<typename Scalar>
class RigidBodyInertia {
public:
  using Vector3 = Eigen::Matrix<Scalar, 3, 1>;
  using Matrix3 = Eigen::Matrix<Scalar, 3, 3>;
  using Matrix6 = Eigen::Matrix<Scalar, 6, 6>;
  using Isometry3 = Eigen::Transform<Scalar, 3, Eigen::Isometry>;

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  RigidBodyInertia() = default;
  RigidBodyInertia(const RigidBodyInertia &) = default;
  RigidBodyInertia(RigidBodyInertia &&) noexcept = default;
  RigidBodyInertia & operator=(const RigidBodyInertia &) = default;
  RigidBodyInertia & operator=(RigidBodyInertia &&) noexcept = default;

  RigidBodyInertia(Scalar mass, Vector3 first_moment, Matrix3 rotational_inertia_origin)
  : mass_(mass),
    first_moment_(std::move(first_moment)),
    rotational_inertia_origin_(std::move(rotational_inertia_origin))
  {
    assert(
      (!detail::is_zero(mass_) || first_moment_.isZero(Scalar(0))) &&
      "RigidBodyInertia with zero mass must have zero first moment");
  }

  [[nodiscard]] static RigidBodyInertia from_center_of_mass(
    const Scalar mass,
    Vector3 com,
    Matrix3 rotational_inertia_com)
  {
    const Vector3 first_moment = mass * com;
    if (detail::is_zero(mass)) {
      return RigidBodyInertia(mass, first_moment, std::move(rotational_inertia_com));
    }
    return RigidBodyInertia(
      mass,
      first_moment,
      std::move(rotational_inertia_com) -
        detail::skew_symmetric(com) * detail::skew_symmetric(first_moment));
  }

  [[nodiscard]] Scalar mass() const noexcept { return mass_; }

  [[nodiscard]] const Vector3 & first_moment() const noexcept { return first_moment_; }

  [[nodiscard]] Vector3 com() const noexcept
  {
    if (detail::is_zero(mass_)) {
      return Vector3::Zero();
    }
    return first_moment_ / mass_;
  }

  [[nodiscard]] const Matrix3 & rotational_inertia() const noexcept
  {
    return rotational_inertia_origin_;
  }

  [[nodiscard]] const Matrix3 & rotational_inertia_origin() const noexcept
  {
    return rotational_inertia_origin_;
  }

  [[nodiscard]] Matrix3 rotational_inertia_com() const noexcept
  {
    if (detail::is_zero(mass_)) {
      return rotational_inertia_origin_;
    }
    return rotational_inertia_origin_ +
           detail::skew_symmetric(first_moment_) * detail::skew_symmetric(first_moment_) / mass_;
  }

  RigidBodyInertia & operator+=(const RigidBodyInertia & other)
  {
    mass_ += other.mass_;
    first_moment_ += other.first_moment_;
    rotational_inertia_origin_ += other.rotational_inertia_origin_;
    return *this;
  }

private:
  Scalar mass_ = Scalar(0);
  Vector3 first_moment_ = Vector3::Zero();
  Matrix3 rotational_inertia_origin_ = Matrix3::Zero();
};

template<typename Scalar>
[[nodiscard]] RigidBodyInertia<Scalar> operator+(
  RigidBodyInertia<Scalar> lhs,
  const RigidBodyInertia<Scalar> & rhs)
{
  lhs += rhs;
  return lhs;
}

template<typename Scalar>
[[nodiscard]] typename RigidBodyInertia<Scalar>::Matrix6 to_matrix(
  const RigidBodyInertia<Scalar> & inertia)
{
  using Matrix3 = typename RigidBodyInertia<Scalar>::Matrix3;
  using Matrix6 = typename RigidBodyInertia<Scalar>::Matrix6;
  Matrix6 matrix = Matrix6::Zero();

  const Matrix3 first_moment_cross = detail::skew_symmetric(inertia.first_moment());

  matrix.template block<3, 3>(0, 0) = inertia.mass() * Matrix3::Identity();
  matrix.template block<3, 3>(0, 3) = -first_moment_cross;
  matrix.template block<3, 3>(3, 0) = first_moment_cross;
  matrix.template block<3, 3>(3, 3) = inertia.rotational_inertia_origin();
  return matrix;
}

template<typename Scalar>
[[nodiscard]] RigidBodyInertia<Scalar> shift(
  const typename RigidBodyInertia<Scalar>::Vector3 & translation_old_in_new,
  const RigidBodyInertia<Scalar> & inertia_in_old)
{
  using Vector3 = typename RigidBodyInertia<Scalar>::Vector3;

  const Scalar mass = inertia_in_old.mass();
  const Vector3 first_moment_new = inertia_in_old.first_moment() + mass * translation_old_in_new;
  if (detail::is_zero(mass)) {
    return RigidBodyInertia<Scalar>(
      mass,
      first_moment_new,
      inertia_in_old.rotational_inertia_origin());
  }
  return RigidBodyInertia<Scalar>(
    mass,
    first_moment_new,
    inertia_in_old.rotational_inertia_com() -
      detail::skew_symmetric(first_moment_new) * detail::skew_symmetric(first_moment_new) / mass);
}

template<typename Scalar>
[[nodiscard]] RigidBodyInertia<Scalar> reexpress(
  const typename RigidBodyInertia<Scalar>::Isometry3 & transform_new_old,
  const RigidBodyInertia<Scalar> & inertia_in_old)
{
  const Scalar mass = inertia_in_old.mass();
  const typename RigidBodyInertia<Scalar>::Vector3 first_moment_new =
    transform_new_old.linear() * inertia_in_old.first_moment() +
    mass * transform_new_old.translation();
  const typename RigidBodyInertia<Scalar>::Matrix3 rotational_inertia_com_new =
    transform_new_old.linear() * inertia_in_old.rotational_inertia_com() *
    transform_new_old.linear().transpose();
  if (detail::is_zero(mass)) {
    return RigidBodyInertia<Scalar>(mass, first_moment_new, rotational_inertia_com_new);
  }

  return RigidBodyInertia<Scalar>(
    mass,
    first_moment_new,
    rotational_inertia_com_new -
      detail::skew_symmetric(first_moment_new) * detail::skew_symmetric(first_moment_new) /
        mass);
}

template<typename Scalar>
[[nodiscard]] RigidBodyInertia<Scalar> add_reexpressed(
  const RigidBodyInertia<Scalar> & base_inertia,
  const typename RigidBodyInertia<Scalar>::Isometry3 & transform_base_other,
  const RigidBodyInertia<Scalar> & other_inertia)
{
  return base_inertia + reexpress(transform_base_other, other_inertia);
}

template<typename Scalar>
[[nodiscard]] Momentum<Scalar> momentum(
  const RigidBodyInertia<Scalar> & inertia,
  const Twist<Scalar> & twist)
{
  return Momentum<Scalar>(to_matrix(inertia) * twist.vector());
}

template<typename Scalar>
[[nodiscard]] Wrench<Scalar> inertial_wrench(
  const RigidBodyInertia<Scalar> & inertia,
  const Twist<Scalar> & twist)
{
  return Wrench<Scalar>(to_matrix(inertia) * twist.vector());
}

using RigidBodyInertiad = RigidBodyInertia<double>;
using RigidBodyInertiaf = RigidBodyInertia<float>;

}  // namespace arm_kinematics

#endif  // ARM_KINEMATICS_RIGID_BODY_INERTIA_HPP
