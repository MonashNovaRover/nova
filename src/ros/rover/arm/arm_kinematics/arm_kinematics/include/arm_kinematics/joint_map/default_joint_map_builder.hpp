//
// Created by Codex on 21/03/2026.
//

#ifndef ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP
#define ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <hardware_interface/component_parser.hpp>
#include <rclcpp/logger.hpp>
#include <tinyxml2.h>
#include <urdf/model.h>

#include "arm_kinematics/joint_map/joint_map_builder.hpp"
#include "arm_kinematics/visibility_control.h"

namespace
{
constexpr const auto kRobotTag = "robot";
constexpr const auto kSDFTag = "sdf";
constexpr const auto kModelTag = "model";
constexpr const auto kROS2ControlTag = "ros2_control";
constexpr const auto kPluginNameTag = "plugin";
constexpr const auto kParamTag = "param";
constexpr const auto kActuatorTag = "actuator";
constexpr const auto kJointTag = "joint";
constexpr const auto kTransmissionTag = "transmission";
constexpr const auto kNameAttribute = "name";
constexpr const auto kRoleAttribute = "role";
constexpr const auto kReductionAttribute = "mechanical_reduction";
constexpr const auto kOffsetAttribute = "offset";
}  // namespace

namespace arm_kinematics {

class ARM_KINEMATICS_PUBLIC DefaultJointMapBuilder final : public JointMapBuilder {
public:
  DefaultJointMapBuilder() = default;

  [[nodiscard]] JointMap build(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names) const override;

  DefaultJointMapBuilder & with_urdf(const urdf::Model & urdf_model);
  DefaultJointMapBuilder & with_transmissions(const std::string & urdf_string, rclcpp::Logger logger);
  DefaultJointMapBuilder & with_transmissions_dangerous(const std::string & urdf_string);

private:
  static std::string get_text_for_element(
    const tinyxml2::XMLElement * element_it, const std::string & tag_name);

  static std::string get_attribute_value(
    const tinyxml2::XMLElement * element_it, const char * attribute_name, std::string tag_name);

  static std::string get_attribute_value(
    const tinyxml2::XMLElement * element_it, const char * attribute_name, const char * tag_name);

  static hardware_interface::JointInfo parse_transmission_joint_from_xml(const tinyxml2::XMLElement * element_it);
  static hardware_interface::ActuatorInfo parse_transmission_actuator_from_xml(const tinyxml2::XMLElement * element_it);

  static double get_parameter_value_or(
    const tinyxml2::XMLElement * params_it, const char * parameter_name, double default_value);

  static std::unordered_map<std::string, std::string> parse_parameters_from_xml(
    const tinyxml2::XMLElement * params_it);

  static hardware_interface::TransmissionInfo parse_transmission_from_xml(
    const tinyxml2::XMLElement * transmission_it);

  std::vector<hardware_interface::TransmissionInfo> transmissions_{};
  std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP
