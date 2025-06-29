//
// Created by nova on 6/29/25.
//

#ifndef SWITCHCONTROLMODECOMMAND_HPP
#define SWITCHCONTROLMODECOMMAND_HPP
#include "Command.hpp"

namespace teleop_arm_joy {

class SwitchControlModeCommand final : public Command {
public:
  void on_initialize(const std::string& prefix, const ParameterInterface::SharedPtr& parameters) override;

  void execute(CommandDelegate& context, const rclcpp::Time& now) override;

protected:
  struct Params {
    std::string to = "";
  };

  Params params_{};
};

} // teleop_arm_joy

#endif //SWITCHCONTROLMODECOMMAND_HPP
