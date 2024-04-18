// Copyright 2020 PAL Robotics SL.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <gmock/gmock.h>
#include <memory>

#include "controller_manager/controller_manager.hpp"
#include "rclcpp/utilities.hpp"
#include "ros2_control_test_assets/descriptions.hpp"
#include "ament_index_cpp/get_package_share_directory.hpp"

TEST(TestLoadPivotDriveController, load_controller)
{
  rclcpp::init(0, nullptr);

  std::shared_ptr<rclcpp::Executor> executor =
    std::make_shared<rclcpp::executors::SingleThreadedExecutor>();

  std::string core_share_path = ament_index_cpp::get_package_share_directory("core");
  std::string urdf_path = core_share_path + "/urdf/rover.urdf.xacro";


  controller_manager::ControllerManager cm(
    std::make_unique<hardware_interface::ResourceManager>(urdf_path),
    executor, "test_controller_manager");

  ASSERT_NE(
    cm.load_controller("test_pivot_drive_controller", "pivot_drive_controller/PivotDriveController"),
    nullptr);

  rclcpp::shutdown();
}
