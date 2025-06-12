//
// Created by nova on 6/11/25.
//

#include "../../../include/teleop_arm_joy/input_sources/joy/JoyInputSource.hpp"

#include "teleop_arm_joy/JoyAxis.hpp"
#include "teleop_arm_joy/JoyButton.hpp"

namespace teleop_arm_joy {

void JoyInputSource::on_initialize() {
  InputSource::on_initialize();

  param_listener_ = std::make_shared<joy_input_source::ParamListener>(node_);
  params_ = param_listener_->get_params();

}

void JoyInputSource::on_configure(InputManager& inputs) {
  if (param_listener_->is_old(params_))
  {
    params_ = param_listener_->get_params();
  }

  // Tidy up any existing elements
  // TODO: Check this actually tidies up the existing elements
  devices.clear();
  buttons.clear();
  axes.clear();

  // Validate that there is at least one device
  if (params_.device_names.empty()) {
    RCLCPP_ERROR(get_node()->get_logger(), "No device names defined!");
    return;
  }

  // Create sinks for axes and buttons that don't get defined in the parameter file.
  // shared_ptr<JoyAxis> sink_axis(new JoyAxis(Params::Axes::MapAxisDefinitions()));
  // for (auto& axis_name : params_.axis_definitions) {
    // axes[axis_name] = sink_axis;
  // }
  // shared_ptr<JoyButton> sink_button(new JoyButton(Params::Buttons::MapButtonDefinitions()));
  // for (auto& button_name : params_.button_definitions) {
  //   buttons[button_name] = sink_button;
  // }

  // Create device instances
  auto device_configs = params_.devices.device_names_map;
  devices.reserve(device_configs.size());

  // Create listener collections for each device, that we can add buttons and axes to later create JoyDevices from.
  auto listeners = map<string, vector<shared_ptr<JoyMessageListener>>*>();
  for (auto& name : params_.device_names) {
    listeners[name] = new vector<shared_ptr<JoyMessageListener>>();
  }
  listeners[""] = listeners[params_.device_names[0]];

  // Create button and axis objects
  RCLCPP_INFO(get_node()->get_logger(), "Registered Buttons:");
  for (auto& [button_name, button_config] : params_.buttons.button_definitions_map) {
    // Buttons without a definition will have their value be -1 by default, so we can filter them out.
    if (button_config.id < 0)
      continue;

    RCLCPP_INFO(node_->get_logger(), "  %s", button_name.c_str());

    shared_ptr<JoyButton> button(new JoyButton(button_config));
    // buttons[button_name] = button;
    listeners[button_config.device]->emplace_back(button);
  }

  RCLCPP_INFO(node_->get_logger(), "Registered Axes:");
  for (auto& [axis_name, axis_config] : params_.axes.axis_definitions_map) {
    // Axes without a definition will have their value be -1 by default, so we can filter them out.
    if (axis_config.id < 0 && axis_config.button_id_negative < 0 && axis_config.button_id_positive < 0)
      continue;

    RCLCPP_INFO(node_->get_logger(), "  %s", axis_name.c_str());

    // shared_ptr<JoyAxis> axis(new JoyAxis(axis_config));
    // axes[axis_name] = axis;
    // listeners[axis_config.device]->emplace_back(axis);
  }

  // Populate unspecified buttons and axes with duds
  // shared_ptr<JoyButton> default_button(new JoyButton(Params::Buttons::MapButtonDefinitions()));
  // for (auto& name : params_.button_definitions) {
  //   inputs.get_booleans().add(name, default_button);
  // }
  //
  // shared_ptr<JoyAxis> default_axis(new JoyAxis(Params::Axes::MapAxisDefinitions()));
  // for (auto& name : params_.button_definitions) {
  //   axes.insert(make_pair(name, default_axis));
  // }

  // Give axes and buttons to a joy device to be managed
  // for (auto& [name, config] : device_configs) {
  //   shared_ptr<JoyDevice> device(new JoyDevice(this, name, config, *listeners[name], std::bind(&TeleopArmJoy::onDeviceUpdated, this, _1)));
  //   devices.emplace_back(device);
  //
  //   // Clean up
  //   delete listeners[name];
  // }
  listeners.clear();
}

} // teleop_arm_joy

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::JoyInputSource, teleop_arm_joy::InputSource);
