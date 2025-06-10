//
// Created by nova on 6/9/25.
//

#ifndef COLLATEDINPUT_HPP
#define COLLATEDINPUT_HPP

#include "Input.hpp"

namespace teleop_arm_joy {
/**
 * This is the class type actually held by the InputManager, which combines multiple other input types.
 */
template<typename T>
class CollatedInput final : public Input<T> {
public:
  T value() override;
  void debounce(const rclcpp::Time& now) override {
    for (auto& input : inputs_)
      input->debounce(now);
  }

  /**
   * @returns true if the value of any input changed since last debounce
   */
  [[nodiscard]] bool changed() const override {
    for (auto& input : inputs_)
      if (input->changed())
        return true;
    return false;
  }

  void add(const std::shared_ptr<Input<T>>& input) {
    inputs_.emplace_back(input);
  }

  void remove(const std::shared_ptr<Input<T>>& input) {
    // Find the element to remove
    const auto it = std::find(inputs_.begin(), inputs_.end(), input);

    // Do nothing if not in events_
    if (it == inputs_.end())
      return;

    // TODO: Do this by swapping with the end and popping to be O(1) rather than O(N)
    inputs_.erase(it);
  }

private:
  std::vector<std::shared_ptr<Input<T>>> inputs_;
};

// value() definitions:
template<>
inline bool CollatedInput<bool>::value() {
  bool sum = false;
  for (const auto& input : inputs_)
    sum = sum || input->value();
  return sum;
}

template<typename T>
T CollatedInput<T>::value() {
  double sum = 0;
  for (const auto& input : inputs_)
    sum += input->value();
  return sum;
}

} // teleop_arm_joy

#endif //COLLATEDINPUT_HPP
