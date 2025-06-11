//
// Created by nova on 6/11/25.
//

#ifndef JOYINPUTSOURCE_HPP
#define JOYINPUTSOURCE_HPP
#include "JoyDevice.hpp"
#include "teleop_arm_joy/input_sources/InputSource.hpp"

// generate_parameter_library_cpp include/teleop_arm_joy/input_sources/joy/joy_input_source_parameters.hpp src/input_sources/joy/joy_input_source_parameters.yaml

namespace teleop_arm_joy {

class JoyInputSource final : public InputSource {
protected:
  void on_initialize() override;
  void on_configure(InputManager& inputs) override;

private:
  std::vector<std::shared_ptr<JoyDevice>> devices;
  std::map<std::string, shared_ptr<JoyButton>> buttons;
  std::map<std::string, shared_ptr<JoyAxis>> axes;

};

} // teleop_arm_joy

#endif //JOYINPUTSOURCE_HPP
