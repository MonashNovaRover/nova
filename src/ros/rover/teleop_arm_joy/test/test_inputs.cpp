//
// Created by nova on 7/1/25.
//

#include <gtest/gtest.h>
#include "teleop_arm_joy/inputs/Button.hpp"
#include "teleop_arm_joy/inputs/Axis.hpp"

using teleop_arm_joy::Button;
using teleop_arm_joy::Axis;

class InputTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code that will be called before each test
  }

  void TearDown() override {
    // Cleanup code that will be called after each test
  }
};

TEST_F(InputTest, ButtonSimple) {
  Button button("test_button");
  EXPECT_FALSE(button.value());

  bool value = false;
  button.add_definition(std::ref(value));
  EXPECT_FALSE(button.value());

  value = true;
  EXPECT_TRUE(button.value());
}

TEST_F(InputTest, ButtonDependencyAccumulation) {
  Button button("test_button");
  EXPECT_FALSE(button.value());

  bool value = false;
  button.add_definition(std::ref(value));
  EXPECT_FALSE(button.value());

  value = true;
  EXPECT_TRUE(button.value());
}

TEST_F(InputTest, AxisSimple) {
  Axis axis("test_axis");
  EXPECT_NEAR(axis.value(), 0.0, 1e-10);

  double value = 1.0;
  axis.add_definition(std::ref(value));
  EXPECT_NEAR(axis.value(), 1.0, 1e-10);

  value = 0.0;
  EXPECT_NEAR(axis.value(), 0.0, 1e-10);

  value = 0.5;
  EXPECT_NEAR(axis.value(), 0.5, 1e-10);
}

TEST_F(InputTest, AxisDependencyAccumulation) {
  Axis axis("test_axis");
  EXPECT_NEAR(axis.value(), 0.0, 1e-10);

  double value = 1.0;
  axis.add_definition(std::ref(value));
  EXPECT_NEAR(axis.value(), 1.0, 1e-10);

  value = 3.0;
  double value2 = 2.0;
  axis.add_definition(std::ref(value2));
  EXPECT_NEAR(axis.value(), 5.0, 1e-10);

  axis.remove_definition(std::ref(value));
  EXPECT_NEAR(axis.value(), 2.0, 1e-10);

  axis.remove_definition(std::ref(value2));
  EXPECT_NEAR(axis.value(), 0.0, 1e-10);
}
