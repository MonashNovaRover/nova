//
// Created by nova on 7/1/25.
//

#include "teleop_arm_joy/inputs/InputCommon.hpp"
#include "teleop_arm_joy/inputs/Button.hpp"
#include "teleop_arm_joy/inputs/Axis.hpp"

namespace teleop_arm_joy {

template<>
bool InputCommon<bool>::value() {
  return accumulate_value();
}

template<>
double InputCommon<double>::value() {
  return accumulate_value();
}

} // namespace teleop_arm_joy
