//
// Created by nova on 6/29/25.
//

#ifndef LOGCOMMAND_HPP
#define LOGCOMMAND_HPP
#include "Command.hpp"

namespace teleop_arm_joy {

class LogCommand final : public Command {
public:
  void on_initialize(const std::string& prefix, const ParameterInterface::SharedPtr& parameters) override;

  void execute(CommandDelegate& context, const rclcpp::Time& now) override;

protected:
  struct Params {
    std::string message = "";
  };

  Params params_{};
};

} // teleop_arm_joy

#endif //LOGCOMMAND_HPP
