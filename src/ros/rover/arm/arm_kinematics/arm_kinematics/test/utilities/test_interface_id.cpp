#include <gtest/gtest.h>

#include "arm_kinematics/utilities/interface_id.hpp"

namespace arm_kinematics
{

TEST(InterfaceIdTest, CommonAccessorsExposeExpectedIds)
{
  EXPECT_EQ(InterfaceId::Position(), InterfaceId{"position"});
  EXPECT_EQ(InterfaceId::Velocity(), InterfaceId{"velocity"});
  EXPECT_EQ(InterfaceId::Acceleration(), InterfaceId{"acceleration"});
  EXPECT_EQ(InterfaceId::Effort(), InterfaceId{"effort"});
}

TEST(InterfaceIdTest, CommonAccessorsReturnStableInstances)
{
  EXPECT_EQ(&InterfaceId::Position(), &InterfaceId::Position());
  EXPECT_EQ(&InterfaceId::Velocity(), &InterfaceId::Velocity());
  EXPECT_EQ(&InterfaceId::Acceleration(), &InterfaceId::Acceleration());
  EXPECT_EQ(&InterfaceId::Effort(), &InterfaceId::Effort());
}

}  // namespace arm_kinematics
