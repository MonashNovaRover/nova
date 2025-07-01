//
// Created by nova on 7/1/25.
//

#include <gtest/gtest.h>
#include "teleop_arm_joy/inputs/Button.hpp"
#include "teleop_arm_joy/inputs/Axis.hpp"
#include "teleop_arm_joy/inputs/InputManager.hpp"

using teleop_arm_joy::Button;
using teleop_arm_joy::Axis;
using teleop_arm_joy::InputManager;

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

TEST_F(InputTest, InputManagerButtonScope) {
  std::cout << "Starting test" << std::endl;
  {
    InputManager inputs;
    std::cout << "Created InputManager" << std::endl;

    bool button_value = true;

    ASSERT_EQ(inputs.get_buttons().size(), 0);
    ASSERT_EQ(inputs.get_events().size(), 0);

    {
      // Get a button, then let it go out of scope
      std::cout << "Getting first button" << std::endl;
      auto button = inputs.get_buttons()["test_button"];
      std::cout << "Got first button" << std::endl;

      ASSERT_EQ(inputs.get_buttons().size(), 1);
      ASSERT_EQ(inputs.get_events().size(), 3);

      EXPECT_FALSE(button->value());
      button->add_definition(std::ref(button_value));
      EXPECT_TRUE(button->value());
      std::cout << "First button scope ending" << std::endl;

      ASSERT_EQ(inputs.get_buttons().size(), 1);
      ASSERT_EQ(inputs.get_events().size(), 3);
    }

    std::cout << "First button gone out of scope" << std::endl;

    // The shared pointers to events should die with the buttons. Thus, as the button goes out of scope, so too should
    // the events it provides, as long as we aren't accidentally holding onto a shared pointer in our test case function
    ASSERT_EQ(inputs.get_buttons().size(), 0);
    ASSERT_EQ(inputs.get_events().size(), 0);

    {
      // This should theoretically be a different button, since we let it go out of scope
      std::cout << "Getting second button" << std::endl;
      auto button = inputs.get_buttons()["test_button"];
      std::cout << "Got second button" << std::endl;

      ASSERT_EQ(inputs.get_buttons().size(), 1);
      ASSERT_EQ(inputs.get_events().size(), 3);
      EXPECT_FALSE(button->value());
      std::cout << "Test completed" << std::endl;
    }

    std::cout << "Second button gone out of scope" << std::endl;

    // Same as before. As it leaves the scope, the collections should become empty.
    ASSERT_EQ(inputs.get_buttons().size(), 0);
    ASSERT_EQ(inputs.get_events().size(), 0);
  }
  std::cout << "InputManager out of scope" << std::endl;
}

TEST_F(InputTest, InputManagerAxisScope) {
  InputManager inputs;

  double axis_value = 1.0;

  {
    // Get an axis, then let it go out of scope
    auto axis = inputs.get_axes()["test_axis"];

    EXPECT_NEAR(axis->value(), 0.0, 1e-10);
    axis->add_definition(std::ref(axis_value));
    EXPECT_NEAR(axis->value(), 1.0, 1e-10);
  }

  {
    // This should theoretically be a different axis, since we let it go out of scope
    auto axis = inputs.get_axes()["test_axis"];
    EXPECT_NEAR(axis->value(), 0.0, 1e-10)
            << "The InputCollection remembered the axis, even though it went out of scope";
  }
}

TEST_F(InputTest, InputManagerButtonEvents) {
  InputManager inputs;
  inputs.update(rclcpp::Time(0));

  bool button_value = false;

  auto button = inputs.get_buttons()["test_button"];

  button->add_definition(std::ref(button_value));
  EXPECT_FALSE(*button);
  EXPECT_FALSE(button->changed()) << "Button change when it was created";

  inputs.update(rclcpp::Time(0));

  EXPECT_FALSE(*button);
  EXPECT_FALSE(button->changed()) << "Button change just after it was created created";

  button_value = true;
  inputs.update(rclcpp::Time(100));

  EXPECT_TRUE(button->changed());
  EXPECT_TRUE(inputs.get_events()["test_button/down"]->is_invoked());
  EXPECT_TRUE(inputs.get_events()["test_button"]->is_invoked());
  EXPECT_FALSE(inputs.get_events()["test_button/up"]->is_invoked());

  button_value = true;
  inputs.update(rclcpp::Time(200));

  EXPECT_FALSE(inputs.get_events()["test_button/down"]->is_invoked());
  EXPECT_FALSE(inputs.get_events()["test_button"]->is_invoked());
  EXPECT_FALSE(inputs.get_events()["test_button/up"]->is_invoked());

  button_value = false;
  inputs.update(rclcpp::Time(300));

  EXPECT_FALSE(inputs.get_events()["test_button/down"]->is_invoked());
  EXPECT_FALSE(inputs.get_events()["test_button"]->is_invoked());
  EXPECT_TRUE(inputs.get_events()["test_button/up"]->is_invoked());

  button_value = true;
  inputs.update(rclcpp::Time(300));
  EXPECT_TRUE(inputs.get_events()["test_button/down"]->is_invoked());
  EXPECT_TRUE(inputs.get_events()["test_button"]->is_invoked());
  EXPECT_FALSE(inputs.get_events()["test_button/up"]->is_invoked());
}
