#include <cmath>

#include <gtest/gtest.h>

#include "arm_kinematics/utilities/utilities.hpp"
#include "arm_kinematics/utilities/wrench.hpp"

namespace arm_kinematics
{
namespace
{

using Vector6d = Eigen::Matrix<double, 6, 1>;

void expect_vec3_near(
  const Eigen::Vector3d & actual,
  const Eigen::Vector3d & expected,
  const double tolerance = 1.0e-12)
{
  EXPECT_TRUE(actual.isApprox(expected, tolerance))
    << "\nactual:\n" << actual << "\nexpected:\n" << expected;
}

}  // namespace

TEST(TwistWrenchTest, StrongTypesMatchUnderlyingEigenStorageSize)
{
  EXPECT_EQ(sizeof(Twistd), sizeof(Vector6d));
  EXPECT_EQ(sizeof(Wrenchd), sizeof(Vector6d));
}

TEST(TwistWrenchTest, TwistConstructsFromEigenVectorAndExposesRepoOrdering)
{
  const Vector6d raw = (Vector6d{} << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0).finished();
  const Twistd twist = raw;

  expect_vec3_near(twist.linear(), Eigen::Vector3d(1.0, 2.0, 3.0));
  expect_vec3_near(twist.angular(), Eigen::Vector3d(4.0, 5.0, 6.0));
}

TEST(TwistWrenchTest, WrenchConstructsFromEigenVectorAndExposesRepoOrdering)
{
  const Vector6d raw = (Vector6d{} << 1.0, 2.0, 3.0, 4.0, 5.0, 6.0).finished();
  const Wrenchd wrench = raw;

  expect_vec3_near(wrench.force(), Eigen::Vector3d(1.0, 2.0, 3.0));
  expect_vec3_near(wrench.moment(), Eigen::Vector3d(4.0, 5.0, 6.0));
}

TEST(TwistWrenchTest, TwistArithmeticRemainsEigenLike)
{
  const Twistd a(Eigen::Vector3d(1.0, 2.0, 3.0), Eigen::Vector3d(4.0, 5.0, 6.0));
  const Twistd b(Eigen::Vector3d(6.0, 5.0, 4.0), Eigen::Vector3d(3.0, 2.0, 1.0));

  const Twistd sum = a + b;

  expect_vec3_near(sum.linear(), Eigen::Vector3d(7.0, 7.0, 7.0));
  expect_vec3_near(sum.angular(), Eigen::Vector3d(7.0, 7.0, 7.0));
}

TEST(TwistWrenchTest, TransformTwistByPureTranslationUsesAdjointForRepoOrdering)
{
  Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
  transform.translation() = Eigen::Vector3d(1.0, 2.0, 3.0);

  const Twistd local(Eigen::Vector3d(4.0, 5.0, 6.0), Eigen::Vector3d(0.5, -1.0, 2.0));
  const Twistd transformed = transform * local;

  expect_vec3_near(
    transformed.linear(),
    local.linear() + transform.translation().cross(local.angular()));
  expect_vec3_near(transformed.angular(), local.angular());
}

TEST(TwistWrenchTest, TransformTwistByPureRotationRotatesBothComponents)
{
  const Eigen::Matrix3d rotation =
    Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitZ()).toRotationMatrix();
  Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
  transform.linear() = rotation;

  const Twistd local(Eigen::Vector3d(1.0, 0.0, 0.0), Eigen::Vector3d(0.0, 1.0, 0.0));
  const Twistd transformed = transform * local;

  expect_vec3_near(transformed.linear(), rotation * local.linear());
  expect_vec3_near(transformed.angular(), rotation * local.angular());
}

TEST(TwistWrenchTest, TransformTwistByRotationAndTranslationMatchesHandFormula)
{
  const Eigen::Matrix3d rotation =
    Eigen::AngleAxisd(M_PI / 3.0, Eigen::Vector3d(0.0, 0.0, 1.0)).toRotationMatrix();
  Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
  transform.linear() = rotation;
  transform.translation() = Eigen::Vector3d(-2.0, 1.5, 0.25);

  const Twistd local(Eigen::Vector3d(0.5, -1.0, 2.0), Eigen::Vector3d(3.0, 0.25, -0.5));
  const Twistd transformed = transform * local;

  const Eigen::Vector3d expected_angular = rotation * local.angular();
  const Eigen::Vector3d expected_linear =
    rotation * local.linear() + transform.translation().cross(expected_angular);

  expect_vec3_near(transformed.linear(), expected_linear);
  expect_vec3_near(transformed.angular(), expected_angular);
}

TEST(TwistWrenchTest, TransformWrenchByPureTranslationUsesDualAdjointForRepoOrdering)
{
  Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
  transform.translation() = Eigen::Vector3d(1.0, 2.0, 3.0);

  const Wrenchd local(Eigen::Vector3d(4.0, 5.0, 6.0), Eigen::Vector3d(0.5, -1.0, 2.0));
  const Wrenchd transformed = transform * local;

  expect_vec3_near(transformed.force(), local.force());
  expect_vec3_near(
    transformed.moment(),
    local.moment() + transform.translation().cross(local.force()));
}

TEST(TwistWrenchTest, TransformWrenchByPureRotationRotatesBothComponents)
{
  const Eigen::Matrix3d rotation =
    Eigen::AngleAxisd(M_PI / 2.0, Eigen::Vector3d::UnitZ()).toRotationMatrix();
  Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
  transform.linear() = rotation;

  const Wrenchd local(Eigen::Vector3d(1.0, 0.0, 0.0), Eigen::Vector3d(0.0, 1.0, 0.0));
  const Wrenchd transformed = transform * local;

  expect_vec3_near(transformed.force(), rotation * local.force());
  expect_vec3_near(transformed.moment(), rotation * local.moment());
}

TEST(TwistWrenchTest, TransformWrenchByRotationAndTranslationMatchesHandFormula)
{
  const Eigen::Matrix3d rotation =
    Eigen::AngleAxisd(M_PI / 3.0, Eigen::Vector3d(0.0, 0.0, 1.0)).toRotationMatrix();
  Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
  transform.linear() = rotation;
  transform.translation() = Eigen::Vector3d(-2.0, 1.5, 0.25);

  const Wrenchd local(Eigen::Vector3d(0.5, -1.0, 2.0), Eigen::Vector3d(3.0, 0.25, -0.5));
  const Wrenchd transformed = transform * local;

  const Eigen::Vector3d expected_force = rotation * local.force();
  const Eigen::Vector3d expected_moment =
    rotation * local.moment() + transform.translation().cross(expected_force);

  expect_vec3_near(transformed.force(), expected_force);
  expect_vec3_near(transformed.moment(), expected_moment);
}

TEST(TwistWrenchTest, PowerPairingMatchesInstantaneousPower)
{
  const Twistd twist(Eigen::Vector3d(1.0, 2.0, 3.0), Eigen::Vector3d(-1.0, 0.5, 4.0));
  const Wrenchd wrench(Eigen::Vector3d(5.0, -2.0, 1.0), Eigen::Vector3d(2.0, 3.0, -0.5));

  EXPECT_DOUBLE_EQ(
    power(wrench, twist),
    wrench.force().dot(twist.linear()) + wrench.moment().dot(twist.angular()));
}

TEST(TwistWrenchTest, CrossMotionMatchesHandDerivedFormula)
{
  const Twistd lhs(Eigen::Vector3d(1.0, 0.0, 0.0), Eigen::Vector3d(0.0, 0.0, 2.0));
  const Twistd rhs(Eigen::Vector3d(0.0, 3.0, 0.0), Eigen::Vector3d(0.0, 5.0, 0.0));

  const Twistd result = cross_motion(lhs, rhs);

  expect_vec3_near(result.linear(), Eigen::Vector3d(-6.0, 0.0, 5.0));
  expect_vec3_near(result.angular(), Eigen::Vector3d(-10.0, 0.0, 0.0));
}

TEST(TwistWrenchTest, CrossForceMatchesHandDerivedFormula)
{
  const Twistd twist(Eigen::Vector3d(1.0, 0.0, 0.0), Eigen::Vector3d(0.0, 0.0, 2.0));
  const Wrenchd wrench(Eigen::Vector3d(0.0, 3.0, 0.0), Eigen::Vector3d(0.0, 5.0, 0.0));

  const Wrenchd result = cross_force(twist, wrench);

  expect_vec3_near(result.force(), Eigen::Vector3d(-6.0, 0.0, 0.0));
  expect_vec3_near(result.moment(), Eigen::Vector3d(-10.0, 0.0, 3.0));
}

TEST(TwistWrenchTest, ApplyTwistPreservesExistingRepoInterpretation)
{
  const Twistd twist(Eigen::Vector3d(1.0, -2.0, 3.0), Eigen::Vector3d(0.0, 0.0, 2.0));
  const double dt = 0.25;

  const Eigen::Isometry3d pose = Eigen::Isometry3d::Identity();
  const Eigen::Isometry3d result = apply_twist(twist, dt, pose);

  expect_vec3_near(result.translation(), twist.linear() * dt);
  EXPECT_TRUE(
    result.linear().isApprox(
      Eigen::AngleAxisd(twist.angular().norm() * dt, twist.angular().normalized()).toRotationMatrix(),
      1.0e-12));
}

}  // namespace arm_kinematics
