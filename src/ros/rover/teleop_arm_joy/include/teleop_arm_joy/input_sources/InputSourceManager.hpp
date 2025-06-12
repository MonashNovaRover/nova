//
// Created by Bailey Chessum on 4/6/25.
//

#ifndef INPUTSOURCEMANAGER_HPP
#define INPUTSOURCEMANAGER_HPP

#include <vector>
#include <pluginlib/class_loader.hpp>
#include <rclcpp/executor.hpp>
#include <rclcpp/node.hpp>

#include "InputSource.hpp"
#include "InputSourceUpdateDelegate.hpp"

namespace teleop_arm_joy
{

class InputSourceManager final : public InputSourceUpdateDelegate, public std::enable_shared_from_this<InputSourceUpdateDelegate> {

public:
  explicit InputSourceManager(const std::shared_ptr<rclcpp::Node>& node, const std::weak_ptr<rclcpp::Executor>& executor)
    : node_(node), executor_(executor) {}

  /**
   * Populates the sources_ from the params in node_.
   */
  void configure(InputManager& inputs);

  /**
   * Gets the control mode plugin class type name for a given input source name, to be given to pluginlib to load.
   * Declares the necessary parameter to get the type name as a side effect.
   * @param[in]  name The name of the input source to get the plugin type name for.
   * @param[out] source_type The output input source plugin type name to be given to pluginlib.
   * @return True if the type name was found. False otherwise.
   */
  bool get_type_for_input_source(const std::string& name, std::string& source_type) const;

  void on_input_source_requested_update(const rclcpp::Time& now) override;

  /**
   * Blocks the current thread until an update is requested by an input source.
   */
  void wait_for_update();

private:
  /// The owning teleop_arm_joy ROS2 node.
  std::shared_ptr<rclcpp::Node> node_;
  /// Add spawned nodes to this to get them to spin
  std::weak_ptr<rclcpp::Executor> executor_;
 
  /// Loads the control modes, and needs to stay alive during the whole lifecycle of the control modes.
  std::unique_ptr<pluginlib::ClassLoader<InputSource>> source_loader_;
  std::vector<std::shared_ptr<InputSource>> sources_{};

  std::mutex mutex_;
  std::condition_variable update_condition_;
  std::atomic<bool> should_update_ = false;
};

}

#endif // INPUTSOURCEMANAGER_HPP
