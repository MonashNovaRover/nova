//
// Created by nova on 6/28/25.
//

#ifndef ACTION_HPP
#define ACTION_HPP
#include <rclcpp/node.hpp>
#include <utility>

#include "CommandDelegate.hpp"

namespace teleop_arm_joy {

/**
 * Abstract base class for a generic invokable action (by an Event) that would change something in teleop
 */
class Command : public EventListener {

public:
  using LoggingInterface = rclcpp::node_interfaces::NodeLoggingInterface;
  using ParameterInterface = rclcpp::node_interfaces::NodeParametersInterface;

  virtual ~Command() = default;
  Command() = default;
  explicit Command(std::string name) : name_(std::move(name)) {}

  /**
   * Sets up the command. Alternative to the constructor, which works with pluginlib.
   * @param name       The name of the command, as defined in the parameter file.
   * @param on         The list of events that should make the command be executed when invoked.
   * @param logging    The logging interface to use.
   * @param parameters The parameter interface to use when configuring the command.
   */
  void initialize(
    const CommandDelegate::WeakPtr& context,
    const std::string& name,
    const std::vector<Event::SharedPtr>& on,
    const LoggingInterface::SharedPtr& logging,
    const ParameterInterface::SharedPtr& parameters);

  /**
   * Allows command implementations to perform implementation specific configuration through the given parameters interface.
   * @param prefix[in]      The prefix string to prepend to all parameter definitions. E.g. "commands.name."
   * @param parameters[in]  Interface to allow the command to get parameters
   */
  virtual void on_initialize(const std::string& prefix, const ParameterInterface::SharedPtr& parameters) = 0;

  /**
   * Method to execute the functionality for the command implementation, called when any of the command's "on" events
   * are invoked.
   * @param context An interface providing access to the internals of Teleop, for the command to mess with.
   * @param now     The time from the input that caused this execution.
   */
  virtual void execute(CommandDelegate& context, const rclcpp::Time& now) = 0;

  /**
   * Called when the "on" events are invoked.
   */
  void on_event_invoked(const rclcpp::Time& now) override;

  /// accessor for the name of the command
  [[nodiscard]] const std::string& get_name() const {
    return name_;
  }

protected:
  /// Accessor for the commands held copied logger object. prevents implementations from modifying the logger
  [[nodiscard]] const rclcpp::Logger& get_logger() {
    if (logger_.has_value())
      return logger_.value();
    logger_ = rclcpp::get_logger(name_);
    return logger_.value();
  }

private:
  /// Name of the command, as defined in the parameter file
  std::string name_ = "uninitialized_command";
  /// Copied logger object from the teleop node
  std::optional<rclcpp::Logger> logger_ = std::nullopt;
  /// List of all events that invoke this command
  std::vector<Event::SharedPtr> on_{};

  CommandDelegate::WeakPtr context_;
};
} // teleop_arm_joy

#endif //ACTION_HPP
