#include <cmath>
#include <type_traits>

#include <gtest/gtest.h>

#include "arm_kinematics/utilities/rigid_body_inertia.hpp"

namespace arm_kinematics
{
namespace
{

constexpr double kPi = 3.14159265358979323846;

Eigen::Matrix3d skew_symmetric(const Eigen::Vector3d & vector)
{
  Eigen::Matrix3d skew;
  skew << 0.0, -vector.z(), vector.y(),
    vector.z(), 0.0, -vector.x(),
    -vector.y(), vector.x(), 0.0;
  return skew;
}

void expect_vec3_near(
  const Eigen::Vector3d & actual,
  const Eigen::Vector3d & expected,
  const double tolerance = 1.0e-12)
{
  EXPECT_TRUE(actual.isApprox(expected, tolerance))
    << "\nactual:\n" << actual << "\nexpected:\n" << expected;
}

void expect_mat3_near(
  const Eigen::Matrix3d & actual,
  const Eigen::Matrix3d & expected,
  const double tolerance = 1.0e-12)
{
  EXPECT_TRUE(actual.isApprox(expected, tolerance))
    << "\nactual:\n" << actual << "\nexpected:\n" << expected;
}

void expect_mat6_near(
  const Eigen::Matrix<double, 6, 6> & actual,
  const Eigen::Matrix<double, 6, 6> & expected,
  const double tolerance = 1.0e-12)
{
  EXPECT_TRUE(actual.isApprox(expected, tolerance))
    << "\nactual:\n" << actual << "\nexpected:\n" << expected;
}

}  // namespace

TEST(RigidBodyInertiaTest, MomentumAliasPreservesWrenchStorageType)
{
  EXPECT_TRUE((std::is_same_v<Momentumd, Wrenchd>));
}

TEST(RigidBodyInertiaTest, OriginFormAccessorsExposeStoredAndDerivedQuantities)
{
  const RigidBodyInertiad inertia(
    2.0,
    Eigen::Vector3d(2.0, -4.0, 6.0),
    (Eigen::Matrix3d{} << 7.0, 0.5, -1.0, 0.5, 8.0, 0.25, -1.0, 0.25, 9.0).finished());

  EXPECT_DOUBLE_EQ(inertia.mass(), 2.0);
  expect_vec3_near(inertia.first_moment(), Eigen::Vector3d(2.0, -4.0, 6.0));
  expect_vec3_near(inertia.com(), Eigen::Vector3d(1.0, -2.0, 3.0));
  expect_mat3_near(
    inertia.rotational_inertia_origin(),
    (Eigen::Matrix3d{} << 7.0, 0.5, -1.0, 0.5, 8.0, 0.25, -1.0, 0.25, 9.0).finished());

  const Eigen::Matrix3d expected_com =
    inertia.rotational_inertia_origin() +
    skew_symmetric(inertia.first_moment()) * skew_symmetric(inertia.first_moment()) /
      inertia.mass();
  expect_mat3_near(inertia.rotational_inertia_com(), expected_com);
}

TEST(RigidBodyInertiaTest, CenterOfMassFactoryRoundTripsExpectedQuantities)
{
  const double mass = 2.5;
  const Eigen::Vector3d com(0.2, -0.3, 0.5);
  const Eigen::Matrix3d rotational_inertia_com =
    (Eigen::Matrix3d{} << 4.0, 0.1, -0.2, 0.1, 5.0, 0.3, -0.2, 0.3, 6.0).finished();

  const RigidBodyInertiad inertia =
    RigidBodyInertiad::from_center_of_mass(mass, com, rotational_inertia_com);

  EXPECT_DOUBLE_EQ(inertia.mass(), mass);
  expect_vec3_near(inertia.first_moment(), mass * com);
  expect_vec3_near(inertia.com(), com);
  expect_mat3_near(inertia.rotational_inertia_com(), rotational_inertia_com);
  expect_mat3_near(
    inertia.rotational_inertia_origin(),
    rotational_inertia_com - skew_symmetric(com) * skew_symmetric(mass * com));
}

TEST(RigidBodyInertiaTest, MatrixFormMatchesRepoOrderingBlocks)
{
  const RigidBodyInertiad inertia(
    3.0,
    Eigen::Vector3d(2.0, -1.0, 4.0),
    (Eigen::Matrix3d{} << 9.0, 0.2, -0.5, 0.2, 10.0, 0.1, -0.5, 0.1, 11.0).finished());

  Eigen::Matrix<double, 6, 6> expected = Eigen::Matrix<double, 6, 6>::Zero();
  expected.block<3, 3>(0, 0) = 3.0 * Eigen::Matrix3d::Identity();
  expected.block<3, 3>(0, 3) = -skew_symmetric(inertia.first_moment());
  expected.block<3, 3>(3, 0) = skew_symmetric(inertia.first_moment());
  expected.block<3, 3>(3, 3) = inertia.rotational_inertia_origin();

  expect_mat6_near(to_matrix(inertia), expected);
}

TEST(RigidBodyInertiaTest, ShiftChangesOriginButPreservesMassComAndInertiaAboutCom)
{
  const RigidBodyInertiad inertia = RigidBodyInertiad::from_center_of_mass(
    2.0,
    Eigen::Vector3d(0.5, -0.25, 1.0),
    (Eigen::Matrix3d{} << 3.0, 0.0, 0.1, 0.0, 4.0, -0.2, 0.1, -0.2, 5.0).finished());
  const Eigen::Vector3d translation_old_in_new(1.0, 2.0, -0.5);

  const RigidBodyInertiad shifted = shift(translation_old_in_new, inertia);

  EXPECT_DOUBLE_EQ(shifted.mass(), inertia.mass());
  expect_vec3_near(shifted.com(), inertia.com() + translation_old_in_new);
  expect_mat3_near(shifted.rotational_inertia_com(), inertia.rotational_inertia_com());
}

TEST(RigidBodyInertiaTest, ReexpressHandlesRotationAndTranslation)
{
  const RigidBodyInertiad inertia = RigidBodyInertiad::from_center_of_mass(
    1.75,
    Eigen::Vector3d(-0.3, 0.4, 0.2),
    (Eigen::Matrix3d{} << 2.0, 0.4, 0.1, 0.4, 3.0, -0.3, 0.1, -0.3, 4.0).finished());

  Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
  transform.linear() =
    Eigen::AngleAxisd(kPi / 4.0, Eigen::Vector3d(0.0, 0.0, 1.0)).toRotationMatrix();
  transform.translation() = Eigen::Vector3d(0.75, -1.25, 0.5);

  const RigidBodyInertiad reexpressed = reexpress(transform, inertia);

  const Eigen::Vector3d expected_com = transform.translation() + transform.linear() * inertia.com();
  const Eigen::Matrix3d expected_inertia_com =
    transform.linear() * inertia.rotational_inertia_com() * transform.linear().transpose();

  EXPECT_DOUBLE_EQ(reexpressed.mass(), inertia.mass());
  expect_vec3_near(reexpressed.com(), expected_com);
  expect_mat3_near(reexpressed.rotational_inertia_com(), expected_inertia_com);
}

TEST(RigidBodyInertiaTest, SameFrameAdditionAndExplicitReexpressedAdditionMatchManualComposition)
{
  const RigidBodyInertiad base = RigidBodyInertiad::from_center_of_mass(
    2.0,
    Eigen::Vector3d(0.1, 0.0, 0.2),
    Eigen::Matrix3d::Identity());
  const RigidBodyInertiad child = RigidBodyInertiad::from_center_of_mass(
    1.0,
    Eigen::Vector3d(0.2, -0.1, 0.3),
    2.0 * Eigen::Matrix3d::Identity());
  Eigen::Isometry3d transform_base_child = Eigen::Isometry3d::Identity();
  transform_base_child.translation() = Eigen::Vector3d(0.5, 0.25, -0.4);

  const RigidBodyInertiad child_in_base = reexpress(transform_base_child, child);
  const RigidBodyInertiad manual_sum = base + child_in_base;
  const RigidBodyInertiad helper_sum = add_reexpressed(base, transform_base_child, child);

  EXPECT_DOUBLE_EQ(manual_sum.mass(), helper_sum.mass());
  expect_vec3_near(manual_sum.first_moment(), helper_sum.first_moment());
  expect_mat3_near(
    manual_sum.rotational_inertia_origin(),
    helper_sum.rotational_inertia_origin());
}

TEST(RigidBodyInertiaTest, MomentumAndInertialWrenchMatchMatrixAction)
{
  const RigidBodyInertiad inertia = RigidBodyInertiad::from_center_of_mass(
    3.0,
    Eigen::Vector3d(0.2, 0.3, -0.1),
    (Eigen::Matrix3d{} << 4.0, -0.2, 0.1, -0.2, 5.0, 0.0, 0.1, 0.0, 6.0).finished());
  const Twistd twist(Eigen::Vector3d(1.0, -2.0, 0.5), Eigen::Vector3d(0.25, 0.75, -1.5));
  const Eigen::Matrix<double, 6, 1> expected = to_matrix(inertia) * twist.vector();

  const Momentumd body_momentum = momentum(inertia, twist);
  const Wrenchd body_inertial_wrench = inertial_wrench(inertia, twist);

  EXPECT_TRUE(body_momentum.vector().isApprox(expected, 1.0e-12));
  EXPECT_TRUE(body_inertial_wrench.vector().isApprox(expected, 1.0e-12));
}

TEST(RigidBodyInertiaTest, ZeroMassInertiaKeepsDerivedQuantitiesWellDefined)
{
  const Eigen::Matrix3d rotational_inertia =
    (Eigen::Matrix3d{} << 1.0, 0.1, 0.0, 0.1, 2.0, 0.2, 0.0, 0.2, 3.0).finished();
  const RigidBodyInertiad inertia(0.0, Eigen::Vector3d::Zero(), rotational_inertia);

  EXPECT_DOUBLE_EQ(inertia.mass(), 0.0);
  expect_vec3_near(inertia.com(), Eigen::Vector3d::Zero());
  expect_mat3_near(inertia.rotational_inertia_com(), rotational_inertia);

  const RigidBodyInertiad shifted = shift(Eigen::Vector3d(1.0, 2.0, 3.0), inertia);
  expect_vec3_near(shifted.first_moment(), Eigen::Vector3d::Zero());
  expect_mat3_near(shifted.rotational_inertia_origin(), rotational_inertia);

  Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
  transform.linear() =
    Eigen::AngleAxisd(kPi / 2.0, Eigen::Vector3d::UnitX()).toRotationMatrix();
  transform.translation() = Eigen::Vector3d(4.0, 5.0, 6.0);
  const RigidBodyInertiad reexpressed = reexpress(transform, inertia);

  expect_vec3_near(reexpressed.first_moment(), Eigen::Vector3d::Zero());
  expect_mat3_near(
    reexpressed.rotational_inertia_origin(),
    transform.linear() * rotational_inertia * transform.linear().transpose());
}

}  // namespace arm_kinematics
