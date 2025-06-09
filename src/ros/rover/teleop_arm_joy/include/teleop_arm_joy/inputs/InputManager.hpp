//
// Created by nova on 6/9/25.
//

#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP
#include <map>
#include <string>

#include "Input.hpp"

namespace teleop_arm_joy {
/**
 * Class responsible for owning the various maps between input name and input object.
 */
class InputManager {


public:

protected:
  std::map<std::string, Input<double>> axes_;
  std::map<std::string, Input<double>> events_;

};

} // teleop_arm_joy

#endif //INPUTMANAGER_HPP
