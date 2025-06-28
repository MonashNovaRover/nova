//
// Created by nova on 6/28/25.
//

#ifndef COMMANDMANAGER_HPP
#define COMMANDMANAGER_HPP
#include "CollatedCommand.hpp"
#include "Command.hpp"

namespace teleop_arm_joy {

class CommandManager final : public CollatedCollection<Command, CollatedCommand> {



};

} // teleop_arm_joy

#endif //COMMANDMANAGER_HPP
