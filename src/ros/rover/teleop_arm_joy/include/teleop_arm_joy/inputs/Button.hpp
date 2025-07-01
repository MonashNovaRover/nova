//
// Created by nova on 7/1/25.
//

#ifndef TELEOP_ARM_JOY_BUTTON_HPP
#define TELEOP_ARM_JOY_BUTTON_HPP

#include <utility>
#include "Input.hpp"

namespace teleop_arm_joy {

class Button final : public InputCommon<bool> {
public:
  using SharedPtr = std::shared_ptr<Button>;
  using WeakPtr = std::weak_ptr<Button>;

  explicit Button(std::string name) : InputCommon<bool>(std::move(name)) {}
};

} // teleop_arm_joy

#endif //TELEOP_ARM_JOY_BUTTON_HPP
