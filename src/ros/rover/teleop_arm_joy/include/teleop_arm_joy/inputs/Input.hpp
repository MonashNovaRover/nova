//
// Created by Bailey Chessum on 4/6/25.
//

#ifndef INPUT_HPP
#define INPUT_HPP

#include <rclcpp/time.hpp>

namespace teleop_arm_joy
{

template<typename T>
class Input {

public:
  explicit Input(const std::string& name) : name_(name) {}
  virtual ~Input() = default;

  virtual T value() = 0;

  virtual void debounce(const rclcpp::Time& now) = 0;

  /**
   * @returns true if the value changed since last debounce
   */
  virtual bool changed() const = 0;

  // Accessors
  [[nodiscard]] const std::string& get_name() const {
    return name_;
  }

private:
  const std::string name_;
};

}

#endif // INPUT_HPP
