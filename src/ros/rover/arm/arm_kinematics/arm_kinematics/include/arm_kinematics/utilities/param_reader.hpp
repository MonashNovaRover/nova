//
// Created by Bailey Chessum on 28/11/2025.
//

#ifndef ARM_KINEMATICS_PARAM_READER_HPP
#define ARM_KINEMATICS_PARAM_READER_HPP

#include <string>
#include <rclcpp/parameter_value.hpp>
#include <rclcpp/node_interfaces/node_parameters_interface.hpp>

namespace arm_kinematics {

/**
 * Utility class for reading params because the default ROS2 api is ass
 */
class ParamReader {
public:
  explicit ParamReader(
      rclcpp::node_interfaces::NodeParametersInterface::SharedPtr params)
      : params_(std::move(params)) {}

  template<typename T>
  T declare_and_get(const std::string & name, const T & default_value) const {
    params_->declare_parameter(name, rclcpp::ParameterValue(default_value));
    return params_->get_parameter(name).get_value<T>();
  }

  template<typename T>
  T get_or(const std::string & name, const T & default_value) const {
    if (!params_->has_parameter(name)) {
      params_->declare_parameter(name, rclcpp::ParameterValue(default_value));
      return default_value;
    }
    return params_->get_parameter(name).get_value<T>();
  }

private:
  rclcpp::node_interfaces::NodeParametersInterface::SharedPtr params_;
};


}

#endif //ARM_KINEMATICS_PARAM_READER_HPP