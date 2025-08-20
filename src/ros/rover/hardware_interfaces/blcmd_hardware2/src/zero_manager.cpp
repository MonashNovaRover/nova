//
// Created by Bailey Chessum on 20/8/25.
//

#include "blcmd_hardware2/zero_manager.hpp"
#include <rclcpp/logging.hpp>

namespace blcmd_hardware {

// Stinky stinky global variable to help share resources if possible
static ZeroManager shared{};

ZeroManager & ZeroManager::get_instance(const std::string& robot_name) {
  std::lock_guard<std::mutex> lock(shared.mtx);

  if (!shared.initialized_) {
    shared.init();
  }

  return shared;
}


double ZeroManager::get_zero(const std::string &name) {

  return 0;
}

void ZeroManager::init(const std::string &robot_name) {
  const auto logger = rclcpp::get_logger("blcmd_hardware2/zero_manager");
  RCLCPP_INFO(logger, "Initializing Zero Manager");

  robot_name_ = "undefined";

  // TODO: Try load zeroes file




  initialized_ = true;
}

}

