//
// Created by nova on 6/28/25.
//

#ifndef COLLATEDCOMMAND_HPP
#define COLLATEDCOMMAND_HPP
#include <memory>

#include "Command.hpp"

namespace teleop_arm_joy {

class CollatedCommand final : public Command {
public:
  explicit CollatedCommand(const std::string& name) {};

  void invoke(CommandDelegate& context) override {
    for (auto command : commands_) {
      if (command)
        command->invoke(context);
    }
  };

  void add(const std::shared_ptr<Command>& event) {
    commands_.emplace_back(event);
  }

  void remove(const std::shared_ptr<Command>& event) {
    // Find the element to remove
    const auto it = std::find(commands_.begin(), commands_.end(), event);

    // Do nothing if not in events_
    if (it == commands_.end())
      return;

    // TODO: Do this by swapping with the end and popping to be O(1) rather than O(N)
    commands_.erase(it);
  }

private:
  std::vector<std::shared_ptr<Command>> commands_{};
};

} // teleop_arm_joy

#endif //COLLATEDCOMMAND_HPP
