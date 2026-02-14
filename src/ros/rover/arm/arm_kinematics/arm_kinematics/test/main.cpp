//
// Created by Bailey Chessum on 15/12/2025.
//
// This file is the entry point for the test cases executable.
//

#include <gtest/gtest.h>
#include <rclcpp/rclcpp/utilities.hpp>
#include <rclcpp/rclcpp/init_options.hpp>

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);

  rclcpp::InitOptions options;
  options.auto_initialize_logging(false);
  options.set_domain_id(222);

  rclcpp::init(argc, argv, options);

  const int ret = RUN_ALL_TESTS();

  rclcpp::shutdown();
  return ret;
}
