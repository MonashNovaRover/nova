//
// Created by nova on 7/1/25.
//

#ifndef TELEOP_ARM_JOY_AXIS_HPP
#define TELEOP_ARM_JOY_AXIS_HPP

#include <utility>
#include "Input.hpp"

namespace teleop_arm_joy {

class Axis : public InputCommon<double> {
public:
  using SharedPtr = std::shared_ptr<Axis>;
  using WeakPtr = std::weak_ptr<Axis>;

  explicit Axis(std::string name) : InputCommon<double>(std::move(name)) {}
};

} // teleop_arm_joy

#endif //TELEOP_ARM_JOY_AXIS_HPP
