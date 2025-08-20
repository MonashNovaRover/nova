//
// Created by Bailey Chessum on 20/8/25.
//

#ifndef BLCMD_HARDWARE2_ZERO_MANAGER_HPP
#define BLCMD_HARDWARE2_ZERO_MANAGER_HPP

#include <mutex>
#include <string>
#include <map>
#include <yaml-cpp/yaml.h>


namespace blcmd_hardware2 {

/**
 * Helper class allowing for persistent software-side zeroing of the BLCMDs.
 * DO NOT USE IN THE REAL TIME LOOP!!!
 */
class ZeroManager {
  std::mutex mtx;

  /// Gets the shared ZeroManager, or initializes SHTO
  static ZeroManager & get_instance(const std::string& robot_name);

  /// Gets the stored zero value for some joint name
  double get_zero(const std::string& name);

private:
  // All private methods can assume prior mutex acquisition.

  /// Sets up the zero manager by reading zero values from disk
  void init(const std::string &robot_name);

  /// Stores all zero valeus that are defined in the zeroes file
  std::map<std::string, double> zeroes_{};
  bool initialized_ = false;
  std::string robot_name_ = "undefined";

  YAML::Node zeroes_yaml_;
};

}  // blcmd_hardware

#endif //BLCMD_HARDWARE2_ZERO_MANAGER_HPP
