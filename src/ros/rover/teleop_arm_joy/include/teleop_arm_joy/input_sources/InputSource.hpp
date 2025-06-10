//
// Created by Bailey Chessum on 4/6/25.
//

#ifndef INPUTSOURCE_HPP
#define INPUTSOURCE_HPP
#include "teleop_arm_joy/inputs/Collection.hpp"
#include "teleop_arm_joy/inputs/Event.hpp"
#include "teleop_arm_joy/inputs/Input.hpp"

namespace teleop_arm_joy
{
/**
 * A base class for various sources of inputs and event invokers, such as joysticks, keyboards, the GUI, etc.
 */
class InputSource {

public:
  virtual ~InputSource() = default;

  virtual void initialize(const std::shared_ptr<rclcpp::Node>& node, const std::string& name) = 0;

  /**
   * Implementers should create, keep reference to, and return any axes provided by the input source here.
   * @return The set of axes the input source exposes.
   */
  virtual std::vector<Input<double>> export_axes() = 0;
  virtual void import_events(Collection<Event>& events) = 0;
};

}

#endif // INPUTSOURCE_HPP
