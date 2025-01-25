// auto-generated DO NOT EDIT

#pragma once

#include <algorithm>
#include <array>
#include <functional>
#include <limits>
#include <mutex>
#include <rclcpp/node.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <rclcpp/logger.hpp>
#include <set>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include <parameter_traits/parameter_traits.hpp>

#include <rsl/static_string.hpp>
#include <rsl/static_vector.hpp>
#include <rsl/parameter_validators.hpp>



namespace teleop_arm_joy {

// Use validators from RSL
using rsl::unique;
using rsl::subset_of;
using rsl::fixed_size;
using rsl::size_gt;
using rsl::size_lt;
using rsl::not_empty;
using rsl::element_bounds;
using rsl::lower_element_bounds;
using rsl::upper_element_bounds;
using rsl::bounds;
using rsl::lt;
using rsl::gt;
using rsl::lt_eq;
using rsl::gt_eq;
using rsl::one_of;
using rsl::to_parameter_result_msg;

// temporarily needed for backwards compatibility for custom validators
using namespace parameter_traits;

template <typename T>
[[nodiscard]] auto to_parameter_value(T value) {
    return rclcpp::ParameterValue(value);
}

template <size_t capacity>
[[nodiscard]] auto to_parameter_value(rsl::StaticString<capacity> const& value) {
    return rclcpp::ParameterValue(rsl::to_string(value));
}

template <typename T, size_t capacity>
[[nodiscard]] auto to_parameter_value(rsl::StaticVector<T, capacity> const& value) {
    return rclcpp::ParameterValue(rsl::to_vector(value));
}
    struct Params {
        std::vector<std::string> devices = {"Left", "Right"};
        std::vector<std::string> axis_definitions = {"j1", "j2", "j3", "j4", "j5", "j6", "speed", "offset"};
        std::vector<std::string> button_definitions = {"limit_on", "limit_off", "lock", "unlock"};
        struct DeviceMappings {
            struct MapDevices {
                std::string topic;
                struct Axes {
                    struct MapAxisDefinitions {
                        int64_t id;
                    };
                    std::map<std::string, MapAxisDefinitions> axis_definitions_map;
                } axes;
                struct Buttons {
                    struct MapButtonDefinitions {
                        int64_t id;
                    };
                    std::map<std::string, MapButtonDefinitions> button_definitions_map;
                } buttons;
            };
            std::map<std::string, MapDevices> devices_map;
        } device_mappings;
        // for detecting if the parameter struct has been updated
        rclcpp::Time __stamp;
    };
    struct StackParams {
    };

  class ParamListener{
  public:


    // throws rclcpp::exceptions::InvalidParameterValueException on initialization if invalid parameter are loaded
    ParamListener(rclcpp::Node::SharedPtr node, std::string const& prefix = "")
    : ParamListener(node->get_node_parameters_interface(), node->get_logger(), prefix) {}

    ParamListener(rclcpp_lifecycle::LifecycleNode::SharedPtr node, std::string const& prefix = "")
    : ParamListener(node->get_node_parameters_interface(), node->get_logger(), prefix) {}

    ParamListener(const std::shared_ptr<rclcpp::node_interfaces::NodeParametersInterface>& parameters_interface,
                  std::string const& prefix = "")
    : ParamListener(parameters_interface, rclcpp::get_logger("teleop_arm_joy"), prefix) {
      RCLCPP_DEBUG(logger_, "ParameterListener: Not using node logger, recommend using other constructors to use a node logger");
    }

    ParamListener(const std::shared_ptr<rclcpp::node_interfaces::NodeParametersInterface>& parameters_interface,
                  rclcpp::Logger logger, std::string const& prefix = "") {
      logger_ = logger;
      prefix_ = prefix;
      if (!prefix_.empty() && prefix_.back() != '.') {
        prefix_ += ".";
      }

      parameters_interface_ = parameters_interface;
      declare_params();
      auto update_param_cb = [this](const std::vector<rclcpp::Parameter> &parameters){return this->update(parameters);};
      handle_ = parameters_interface_->add_on_set_parameters_callback(update_param_cb);
      clock_ = rclcpp::Clock();
    }

    Params get_params() const{
      std::lock_guard<std::mutex> lock(mutex_);
      return params_;
    }

    bool is_old(Params const& other) const {
      std::lock_guard<std::mutex> lock(mutex_);
      return params_.__stamp != other.__stamp;
    }

    StackParams get_stack_params() {
      Params params = get_params();
      StackParams output;


      return output;
    }

    void refresh_dynamic_parameters() {
      auto updated_params = get_params();
      // TODO remove any destroyed dynamic parameters

      // declare any new dynamic parameters
      rclcpp::Parameter param;
      for (const auto & value_1 : updated_params.devices) {

          auto& entry = updated_params.device_mappings.devices_map[value_1];
          std::string value = fmt::format("{}", value_1);

          auto param_name = fmt::format("{}{}.{}.{}", prefix_, "device_mappings", value, "topic");
          if (!parameters_interface_->has_parameter(param_name)) {
              rcl_interfaces::msg::ParameterDescriptor descriptor;
              descriptor.description = "The topic to get sensor_msgs/msg/Joy messages from for this device";
              descriptor.read_only = false;
              auto parameter = rclcpp::ParameterType::PARAMETER_STRING;
              parameters_interface_->declare_parameter(param_name, parameter, descriptor);
          }
          param = parameters_interface_->get_parameter(param_name);
          RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
          entry.topic = param.as_string();}

      for (const auto & value_1 : updated_params.devices) {
      for (const auto & value_2 : updated_params.axis_definitions) {

          auto& entry = updated_params.device_mappings.__map_devices.devices_map[value_1].axis_definitions_map[value_2];
          std::string value = fmt::format("{}.{}", value_1, value_2);

          auto param_name = fmt::format("{}{}.{}.{}", prefix_, "device_mappings.__map_devices", value, "id");
          if (!parameters_interface_->has_parameter(param_name)) {
              rcl_interfaces::msg::ParameterDescriptor descriptor;
              descriptor.description = "";
              descriptor.read_only = false;
              auto parameter = rclcpp::ParameterType::PARAMETER_INTEGER;
              parameters_interface_->declare_parameter(param_name, parameter, descriptor);
          }
          param = parameters_interface_->get_parameter(param_name);
          RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
          entry.id = param.as_int();}
      }

      for (const auto & value_1 : updated_params.devices) {
      for (const auto & value_2 : updated_params.button_definitions) {

          auto& entry = updated_params.device_mappings.__map_devices.devices_map[value_1].button_definitions_map[value_2];
          std::string value = fmt::format("{}.{}", value_1, value_2);

          auto param_name = fmt::format("{}{}.{}.{}", prefix_, "device_mappings.__map_devices", value, "id");
          if (!parameters_interface_->has_parameter(param_name)) {
              rcl_interfaces::msg::ParameterDescriptor descriptor;
              descriptor.description = "";
              descriptor.read_only = false;
              auto parameter = rclcpp::ParameterType::PARAMETER_INTEGER;
              parameters_interface_->declare_parameter(param_name, parameter, descriptor);
          }
          param = parameters_interface_->get_parameter(param_name);
          RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
          entry.id = param.as_int();}
      }

    }

    rcl_interfaces::msg::SetParametersResult update(const std::vector<rclcpp::Parameter> &parameters) {
      auto updated_params = get_params();

      for (const auto &param: parameters) {
        if (param.get_name() == (prefix_ + "devices")) {
            updated_params.devices = param.as_string_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "axis_definitions")) {
            updated_params.axis_definitions = param.as_string_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
        if (param.get_name() == (prefix_ + "button_definitions")) {
            updated_params.button_definitions = param.as_string_array();
            RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
        }
      }

      // update dynamic parameters
      for (const auto &param: parameters) {
        for (const auto & value_1 : updated_params.devices) {
        std::string value = fmt::format("{}", value_1);

            auto param_name = fmt::format("{}{}.{}.{}", prefix_, "device_mappings", value, "topic");
            if (param.get_name() == param_name) {

                updated_params.device_mappings.devices_map[value_1].topic = param.as_string();
                RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
            }
        }

        for (const auto & value_1 : updated_params.devices) {
        for (const auto & value_2 : updated_params.axis_definitions) {
        std::string value = fmt::format("{}.{}", value_1, value_2);

            auto param_name = fmt::format("{}{}.{}.{}", prefix_, "device_mappings.__map_devices", value, "id");
            if (param.get_name() == param_name) {

                updated_params.device_mappings.__map_devices.devices_map[value_1].axis_definitions_map[value_2].id = param.as_int();
                RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
            }
        }
        }

        for (const auto & value_1 : updated_params.devices) {
        for (const auto & value_2 : updated_params.button_definitions) {
        std::string value = fmt::format("{}.{}", value_1, value_2);

            auto param_name = fmt::format("{}{}.{}.{}", prefix_, "device_mappings.__map_devices", value, "id");
            if (param.get_name() == param_name) {

                updated_params.device_mappings.__map_devices.devices_map[value_1].button_definitions_map[value_2].id = param.as_int();
                RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
            }
        }
        }

      }
      updated_params.__stamp = clock_.now();
      update_internal_params(updated_params);
      return rsl::to_parameter_result_msg({});
    }

    void declare_params(){
      auto updated_params = get_params();
      // declare all parameters and give default values to non-required ones
      if (!parameters_interface_->has_parameter(prefix_ + "devices")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "";
          descriptor.read_only = false;
          auto parameter = to_parameter_value(updated_params.devices);
          parameters_interface_->declare_parameter(prefix_ + "devices", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "axis_definitions")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "The list of mappable axes.";
          descriptor.read_only = true;
          auto parameter = to_parameter_value(updated_params.axis_definitions);
          parameters_interface_->declare_parameter(prefix_ + "axis_definitions", parameter, descriptor);
      }
      if (!parameters_interface_->has_parameter(prefix_ + "button_definitions")) {
          rcl_interfaces::msg::ParameterDescriptor descriptor;
          descriptor.description = "The list of mappable axes.";
          descriptor.read_only = true;
          auto parameter = to_parameter_value(updated_params.button_definitions);
          parameters_interface_->declare_parameter(prefix_ + "button_definitions", parameter, descriptor);
      }
      // get parameters and fill struct fields
      rclcpp::Parameter param;
      param = parameters_interface_->get_parameter(prefix_ + "devices");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.devices = param.as_string_array();
      param = parameters_interface_->get_parameter(prefix_ + "axis_definitions");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.axis_definitions = param.as_string_array();
      param = parameters_interface_->get_parameter(prefix_ + "button_definitions");
      RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
      updated_params.button_definitions = param.as_string_array();


      // declare and set all dynamic parameters
      for (const auto & value_1 : updated_params.devices) {

          auto& entry = updated_params.device_mappings.devices_map[value_1];
          std::string value = fmt::format("{}", value_1);

          auto param_name = fmt::format("{}{}.{}.{}", prefix_, "device_mappings", value, "topic");
          if (!parameters_interface_->has_parameter(param_name)) {
              rcl_interfaces::msg::ParameterDescriptor descriptor;
              descriptor.description = "The topic to get sensor_msgs/msg/Joy messages from for this device";
              descriptor.read_only = false;
              auto parameter = rclcpp::ParameterType::PARAMETER_STRING;
              parameters_interface_->declare_parameter(param_name, parameter, descriptor);
          }
          param = parameters_interface_->get_parameter(param_name);
          RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
          entry.topic = param.as_string();}

      for (const auto & value_1 : updated_params.devices) {
      for (const auto & value_2 : updated_params.axis_definitions) {

          auto& entry = updated_params.device_mappings.__map_devices.devices_map[value_1].axis_definitions_map[value_2];
          std::string value = fmt::format("{}.{}", value_1, value_2);

          auto param_name = fmt::format("{}{}.{}.{}", prefix_, "device_mappings.__map_devices", value, "id");
          if (!parameters_interface_->has_parameter(param_name)) {
              rcl_interfaces::msg::ParameterDescriptor descriptor;
              descriptor.description = "";
              descriptor.read_only = false;
              auto parameter = rclcpp::ParameterType::PARAMETER_INTEGER;
              parameters_interface_->declare_parameter(param_name, parameter, descriptor);
          }
          param = parameters_interface_->get_parameter(param_name);
          RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
          entry.id = param.as_int();}
      }

      for (const auto & value_1 : updated_params.devices) {
      for (const auto & value_2 : updated_params.button_definitions) {

          auto& entry = updated_params.device_mappings.__map_devices.devices_map[value_1].button_definitions_map[value_2];
          std::string value = fmt::format("{}.{}", value_1, value_2);

          auto param_name = fmt::format("{}{}.{}.{}", prefix_, "device_mappings.__map_devices", value, "id");
          if (!parameters_interface_->has_parameter(param_name)) {
              rcl_interfaces::msg::ParameterDescriptor descriptor;
              descriptor.description = "";
              descriptor.read_only = false;
              auto parameter = rclcpp::ParameterType::PARAMETER_INTEGER;
              parameters_interface_->declare_parameter(param_name, parameter, descriptor);
          }
          param = parameters_interface_->get_parameter(param_name);
          RCLCPP_DEBUG_STREAM(logger_, param.get_name() << ": " << param.get_type_name() << " = " << param.value_to_string());
          entry.id = param.as_int();}
      }


      updated_params.__stamp = clock_.now();
      update_internal_params(updated_params);
    }

    private:
      void update_internal_params(Params updated_params) {
        std::lock_guard<std::mutex> lock(mutex_);
        params_ = updated_params;
      }

      std::string prefix_;
      Params params_;
      rclcpp::Clock clock_;
      std::shared_ptr<rclcpp::node_interfaces::OnSetParametersCallbackHandle> handle_;
      std::shared_ptr<rclcpp::node_interfaces::NodeParametersInterface> parameters_interface_;

      // rclcpp::Logger cannot be default-constructed
      // so we must provide a initialization here even though
      // every one of our constructors initializes logger_
      rclcpp::Logger logger_ = rclcpp::get_logger("teleop_arm_joy");
      std::mutex mutable mutex_;
  };

} // namespace teleop_arm_joy
