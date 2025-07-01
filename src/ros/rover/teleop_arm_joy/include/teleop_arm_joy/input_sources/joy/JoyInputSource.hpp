//
// Created by nova on 6/11/25.
//

#ifndef JOYINPUTSOURCE_HPP
#define JOYINPUTSOURCE_HPP

#include "teleop_arm_joy/input_sources/InputSource.hpp"
#include "joy_input_source_parameters.hpp"

namespace teleop_arm_joy {

class JoyInputSource final : public InputSource {
protected:
  void on_initialize() override;
  void on_update(const rclcpp::Time& now) override;

  void export_buttons(std::vector<InputDeclaration<bool>>& declarations) override;
  void export_axes(std::vector<InputDeclaration<double>>& definitions) override;

private:
  std::shared_ptr<joy_input_source::ParamListener> param_listener_;
  joy_input_source::Params params_;

  struct JoyAxis {
    using AxisParams = joy_input_source::Params::Axes::MapAxisDefinitions;

    double value;
    std::string name;
    AxisParams params;

    JoyAxis(const std::string& name, AxisParams _params) : name(name), params(_params) {}
  };

  struct JoyButton {
    using ButtonParams = joy_input_source::Params::Buttons::MapButtonDefinitions;

    bool value;
    std::string name;
    ButtonParams params;

    JoyButton(const std::string& name, ButtonParams _params) : name(name), params(_params) {}
  };

  std::vector<JoyAxis> axes_;
  std::vector<JoyButton> buttons_;
};

} // teleop_arm_joy

#endif //JOYINPUTSOURCE_HPP
