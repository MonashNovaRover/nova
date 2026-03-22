//
// Created by Bailey Chessum on 25/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis_import.hpp"

#include <rclcpp/logging.hpp>
#include <tinyxml2.h>

#include <charconv>
#include <cstring>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#include <utility>

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

namespace {

class Ros2ControlTransmissionModel final : public TransmissionModel {
public:
  [[nodiscard]] bool can_build(
    const JointQuantity,
    const PropagationDirection) const noexcept override
  {
    return false;
  }

  [[nodiscard]] std::unique_ptr<const ComputeTransmission> build(
    const JointQuantity,
    const PropagationDirection,
    span<const JointId>,
    span<const JointId>) const override
  {
    throw std::logic_error("Ros2ControlTransmissionModel::build() called before transmission runtime support exists");
  }
};

std::string get_text_for_element(
  const tinyxml2::XMLElement * element_it, const std::string &)
{
  const auto get_text_output = element_it->GetText();
  if (!get_text_output)
    return "";
  return get_text_output;
}

std::string get_attribute_value(
  const tinyxml2::XMLElement * element_it, const char * attribute_name, const std::string & tag_name)
{
  const tinyxml2::XMLAttribute * attr = element_it->FindAttribute(attribute_name);
  if (!attr)
    throw std::runtime_error(std::string("no attribute ") + attribute_name + " in " + tag_name + " tag");
  return element_it->Attribute(attribute_name);
}

std::string get_attribute_value(
  const tinyxml2::XMLElement * element_it, const char * attribute_name, const char * tag_name)
{
  return get_attribute_value(element_it, attribute_name, std::string(tag_name));
}

namespace impl
{
std::optional<double> stod(const std::string & s)
{
#if __cplusplus < 202002L
  std::istringstream stream(s);
  stream.imbue(std::locale::classic());
  double result;
  stream >> result;
  if (stream.fail() || !stream.eof())
  {
    return std::nullopt;
  }
  return result;
#else
  double result_value;
  const auto parse_result = std::from_chars(s.data(), s.data() + s.size(), result_value);
  if (parse_result.ec == std::errc())
  {
    return result_value;
  }
  return std::nullopt;
#endif
}
}  // namespace impl

double stod(const std::string & s)
{
  if (const auto result = impl::stod(s))
  {
    return *result;
  }
  throw std::invalid_argument("Failed converting string to real number");
}

double get_parameter_value_or(
  const tinyxml2::XMLElement * params_it, const char * parameter_name, const double default_value)
{
  while (params_it)
  {
    try
    {
      const auto tag_name = params_it->Name();
      if (strcmp(tag_name, parameter_name) == 0)
      {
        const auto tag_text = params_it->GetText();
        if (tag_text)
        {
          return stod(tag_text);
        }
      }
    }
    catch (const std::exception &)
    {
      return default_value;
    }

    params_it = params_it->NextSiblingElement();
  }

  return default_value;
}

std::unordered_map<std::string, std::string> parse_parameters_from_xml(
  const tinyxml2::XMLElement * params_it)
{
  std::unordered_map<std::string, std::string> parameters;

  while (params_it)
  {
    const tinyxml2::XMLAttribute * attr = params_it->FindAttribute(kNameAttribute);
    if (!attr)
    {
      throw std::runtime_error("no parameter name attribute set in param tag");
    }
    const std::string parameter_name = params_it->Attribute(kNameAttribute);
    const std::string parameter_value = get_text_for_element(params_it, parameter_name);
    parameters[parameter_name] = parameter_value;

    params_it = params_it->NextSiblingElement(kParamTag);
  }
  return parameters;
}

hardware_interface::JointInfo parse_transmission_joint_from_xml(const tinyxml2::XMLElement * element_it)
{
  hardware_interface::JointInfo joint_info;
  joint_info.name = get_attribute_value(element_it, kNameAttribute, element_it->Name());
  joint_info.role = get_attribute_value(element_it, kRoleAttribute, element_it->Name());
  joint_info.mechanical_reduction =
    get_parameter_value_or(element_it->FirstChildElement(), kReductionAttribute, 1.0);
  joint_info.offset =
    get_parameter_value_or(element_it->FirstChildElement(), kOffsetAttribute, 0.0);
  return joint_info;
}

hardware_interface::ActuatorInfo parse_transmission_actuator_from_xml(const tinyxml2::XMLElement * element_it)
{
  hardware_interface::ActuatorInfo actuator_info;
  actuator_info.name = get_attribute_value(element_it, kNameAttribute, element_it->Name());
  actuator_info.role = get_attribute_value(element_it, kRoleAttribute, element_it->Name());
  actuator_info.mechanical_reduction =
    get_parameter_value_or(element_it->FirstChildElement(), kReductionAttribute, 1.0);
  actuator_info.offset =
    get_parameter_value_or(element_it->FirstChildElement(), kOffsetAttribute, 0.0);
  return actuator_info;
}

hardware_interface::TransmissionInfo parse_transmission_from_xml(const tinyxml2::XMLElement * transmission_it)
{
  hardware_interface::TransmissionInfo transmission;

  transmission.name = get_attribute_value(transmission_it, kNameAttribute, transmission_it->Name());
  const auto * type_it = transmission_it->FirstChildElement(kPluginNameTag);
  if (!type_it)
  {
    throw std::runtime_error("Missing <plugin> tag of <transmission> element in your URDF.");
  }
  transmission.type = get_text_for_element(type_it, kPluginNameTag);

  const auto * joint_it = transmission_it->FirstChildElement(kJointTag);
  while (joint_it)
  {
    transmission.joints.push_back(parse_transmission_joint_from_xml(joint_it));
    joint_it = joint_it->NextSiblingElement(kJointTag);
  }

  const auto * actuator_it = transmission_it->FirstChildElement(kActuatorTag);
  while (actuator_it)
  {
    transmission.actuators.push_back(parse_transmission_actuator_from_xml(actuator_it));
    actuator_it = actuator_it->NextSiblingElement(kActuatorTag);
  }

  const auto * params_it = transmission_it->FirstChildElement(kParamTag);
  if (params_it)
  {
    transmission.parameters = parse_parameters_from_xml(params_it);
  }

  return transmission;
}

void add_ros2_control_transmission_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const hardware_interface::TransmissionInfo & transmission)
{
  const auto model_id = transmission_analysis.add_model(std::make_unique<Ros2ControlTransmissionModel>());

  std::vector<std::string> input_names{};
  input_names.reserve(transmission.actuators.size());
  for (const auto & actuator : transmission.actuators) {
    input_names.emplace_back(actuator.name);
  }

  std::vector<std::string> output_names{};
  output_names.reserve(transmission.joints.size());
  for (const auto & joint : transmission.joints) {
    output_names.emplace_back(joint.name);
  }

  transmission_analysis.add_transmission(
    model_id,
    span<const std::string>(input_names),
    span<const std::string>(output_names),
    transmission.name);
}

} // namespace

namespace {

enum class MimicVisitState {
  Unvisited,
  Visiting,
  Done
};

TransmissionAnalysis::AffineTransmission normalize_affine_transmission(
  const JointId target_joint_id,
  const std::unordered_map<JointId, TransmissionAnalysis::AffineTransmission> & raw_affine_transmissions,
  std::unordered_map<JointId, TransmissionAnalysis::AffineTransmission> & normalized_affine_transmissions,
  std::unordered_map<JointId, MimicVisitState> & visit_states)
{
  const auto normalized_it = normalized_affine_transmissions.find(target_joint_id);
  if (normalized_it != normalized_affine_transmissions.end()) {
    return normalized_it->second;
  }

  const auto raw_it = raw_affine_transmissions.find(target_joint_id);
  if (raw_it == raw_affine_transmissions.end()) {
    return TransmissionAnalysis::AffineTransmission{
      target_joint_id,
      target_joint_id,
      1.0F,
      0.0F
    };
  }

  const auto visit_state = visit_states[target_joint_id];
  if (visit_state == MimicVisitState::Visiting) {
    throw std::runtime_error("Cycle detected while normalizing mimic transmissions");
  }

  visit_states[target_joint_id] = MimicVisitState::Visiting;

  const auto normalized_source = normalize_affine_transmission(
    raw_it->second.source_joint_id,
    raw_affine_transmissions,
    normalized_affine_transmissions,
    visit_states);

  const auto normalized = TransmissionAnalysis::AffineTransmission{
    target_joint_id,
    normalized_source.source_joint_id,
    normalized_source.multiplier * raw_it->second.multiplier,
    normalized_source.offset * raw_it->second.multiplier + raw_it->second.offset
  };

  visit_states[target_joint_id] = MimicVisitState::Done;
  normalized_affine_transmissions[target_joint_id] = normalized;
  return normalized;
}

} // namespace

std::vector<hardware_interface::TransmissionInfo> add_ros2_control_transmissions_to_analysis_dangerous(
  TransmissionAnalysis & transmission_analysis,
  const std::string & urdf_string)
{
  tinyxml2::XMLDocument doc;
  doc.Parse(urdf_string.c_str());
  if (doc.Error())
    throw std::runtime_error(std::string("invalid URDF passed in to robot parser: ") + doc.ErrorStr());

  const tinyxml2::XMLElement * robot_it = doc.RootElement();
  const tinyxml2::XMLElement * ros2_control_it;

  if (std::string(kRobotTag) == robot_it->Name())
  {
    ros2_control_it = robot_it->FirstChildElement(kROS2ControlTag);
  }
  else if (std::string(kSDFTag) == robot_it->Name())
  {
    const tinyxml2::XMLElement * model_it = robot_it->FirstChildElement(kModelTag);
    ros2_control_it = model_it->FirstChildElement(kROS2ControlTag);
  }
  else
  {
    throw std::runtime_error(
      "the robot tag is not root element in URDF or sdf tag is not root element in SDF");
  }

  if (!ros2_control_it)
    throw std::runtime_error(std::string("no ") + kROS2ControlTag + " tag");

  std::vector<hardware_interface::TransmissionInfo> transmissions{};

  while (ros2_control_it)
  {
    const auto * ros2_control_child_it = ros2_control_it->FirstChildElement();

    while (ros2_control_child_it) {
      if (std::string(kTransmissionTag) == ros2_control_child_it->Name()) {
        auto transmission = parse_transmission_from_xml(ros2_control_child_it);
        add_ros2_control_transmission_to_analysis(transmission_analysis, transmission);
        transmissions.push_back(std::move(transmission));
      }

      ros2_control_child_it = ros2_control_child_it->NextSiblingElement();
    }

    ros2_control_it = ros2_control_it->NextSiblingElement(kROS2ControlTag);
  }

  return transmissions;
}

std::vector<hardware_interface::TransmissionInfo> add_ros2_control_transmissions_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const std::string & urdf_string,
  rclcpp::Logger logger)
{
  try {
    return add_ros2_control_transmissions_to_analysis_dangerous(transmission_analysis, urdf_string);
  } catch (const std::runtime_error & e) {
    RCLCPP_WARN(logger, "Error trying to add transmissions to TransmissionAnalysis: %s", e.what());
    return {};
  }
}

void add_mimic_transmissions_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const urdf::Model & urdf_model)
{
  std::unordered_map<JointId, TransmissionAnalysis::AffineTransmission> raw_affine_transmissions{};

  for (const auto & [joint_name, joint] : urdf_model.joints_) {
    if (!joint->mimic) {
      continue;
    }

    const auto target_joint_id = transmission_analysis.ensure_joint_id(joint_name);

    raw_affine_transmissions[target_joint_id] = TransmissionAnalysis::AffineTransmission{
      target_joint_id,
      transmission_analysis.ensure_joint_id(joint->mimic->joint_name),
      static_cast<float>(joint->mimic->multiplier),
      static_cast<float>(joint->mimic->offset)
    };
  }

  std::unordered_map<JointId, TransmissionAnalysis::AffineTransmission> normalized_affine_transmissions{};
  std::unordered_map<JointId, MimicVisitState> visit_states{};

  for (const auto & [target_joint_id, _] : raw_affine_transmissions) {
    const auto normalized = normalize_affine_transmission(
      target_joint_id,
      raw_affine_transmissions,
      normalized_affine_transmissions,
      visit_states);

    transmission_analysis.add_affine_transmission(
      normalized.source_joint_id,
      normalized.target_joint_id,
      normalized.multiplier,
      normalized.offset);
  }
}

} // namespace arm_kinematics
