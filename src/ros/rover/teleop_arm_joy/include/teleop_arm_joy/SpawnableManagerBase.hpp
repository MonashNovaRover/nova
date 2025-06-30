//
// Created by nova on 6/30/25.
//

#ifndef TELEOP_ARM_JOY_SPAWNABLEMANAGERBASE_HPP
#define TELEOP_ARM_JOY_SPAWNABLEMANAGERBASE_HPP

namespace teleop_arm_joy {

struct SpawnableBaseParams {
  std::string type;
};


/**
 * Parent class of ControlModeManager, InputSourceManager, created to help reduce code duplication, particularly for the
 * logging functionality.
 */
template<typename T, typename ParamsT>
class SpawnableManagerBase {
  static_assert(std::is_base_of<SpawnableBaseParams, ParamsT>::value, "ParamsT must derive from SpawnableBaseParams.");

  std::shared_ptr<T> operator[](const std::string& index) {
    return items_[index];
  };

  // Make the manager iterable
  using iterator = typename std::map<std::string, std::shared_ptr<T>>::iterator;
  using const_iterator = typename std::map<std::string, std::shared_ptr<T>>::const_iterator;
  iterator begin() override { return items_.begin(); };
  [[nodiscard]] const_iterator begin() const override { return items_.begin(); };
  iterator end() override { return items_.end(); };
  [[nodiscard]] const_iterator end() const override { return items_.end(); };

protected:
  /**
   * This stores the necessary information to be able to give a printout on the status of loading each control mode.
   */
  struct SpawnLog {
    /// The type name that was attempted to be spawned. Empty if one could not be found.
    std::optional<std::string> type;
    /// The message describing how loading failed. Empty on a successful load
    std::optional<std::string> failure_message;
  };

  /// The owning teleop_arm_joy ROS2 node.
  std::shared_ptr<rclcpp::Node> node_;
  /// Add spawned nodes to this to get them to spin
  std::weak_ptr<rclcpp::Executor> executor_;

  /// Loads the T items, and needs to stay alive during the whole lifecycle of the items.
  std::unique_ptr<pluginlib::ClassLoader<ControlMode>> control_mode_loader_;

  /// Currently loaded control modes/input sources.
  std::map<std::string, std::shared_ptr<ControlMode>> items_{};



};

} // teleop_arm_joy

#endif //TELEOP_ARM_JOY_SPAWNABLEMANAGERBASE_HPP
