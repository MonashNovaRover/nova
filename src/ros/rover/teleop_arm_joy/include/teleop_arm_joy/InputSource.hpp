//
// Created by Bailey Chessum on 4/6/25.
//

#ifndef INPUTSOURCE_HPP
#define INPUTSOURCE_HPP

#include <rclcpp/time.hpp>

namespace teleop_arm_joy
{

class InputSource {

public:
  virtual void initialize() = 0;
  virtual void export_buttons() = 0;



};

}

#endif // INPUTSOURCE_HPP
