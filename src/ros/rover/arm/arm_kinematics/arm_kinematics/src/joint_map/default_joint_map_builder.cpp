//
// Created by Bailey Chessum on 21/03/2026.
//

#include "arm_kinematics/joint_map/default_joint_map_builder.hpp"

#include "arm_kinematics/joint_map/affine_joint_map.hpp"

#include <rclcpp/logging.hpp>

#include <charconv>
#include <cstring>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace arm_kinematics {

namespace {

struct BoundaryJointIds {
  std::vector<JointId> ids{};
  size_t known_count = 0;
};

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

std::string to_string(const JointQuantity quantity)
{
  switch (quantity) {
    case JointQuantity::Position:
      return "position";
    case JointQuantity::Velocity:
      return "velocity";
  }

  return "unknown";
}

BoundaryJointIds convert_joint_names_to_ids(
  const Order<std::string, JointId> & joint_ids,
  const std::vector<std::string> & joint_names)
{
  BoundaryJointIds result{};
  result.ids.reserve(joint_names.size());

  for (const auto & name : joint_names) {
    if (!joint_ids.contains_key(name))
      continue;

    result.ids.emplace_back(joint_ids[name]);
    ++result.known_count;
  }

  return result;
}

} // namespace

tl::expected<JointMap, std::string> DefaultJointMapBuilder::build_expected(
  const std::vector<std::string> & input_names,
  const std::vector<std::string> & output_names,
  const JointQuantity quantity) const
{
  try {
    return JointMap(AffineJointMap(input_names, output_names, mimic_joints_));
  } catch (const std::exception & e) {
    const auto & joint_ids = transmission_analysis_.joint_order();
    const auto input_ids = convert_joint_names_to_ids(joint_ids, input_names);
    const auto output_ids = convert_joint_names_to_ids(joint_ids, output_names);

    const bool touches_transmission_analysis =
      input_ids.known_count > 0 || output_ids.known_count > 0;

    if (touches_transmission_analysis && !transmission_analysis_.transmissions().empty()) {
      return tl::make_unexpected(
        "Transmission-backed JointMap build for " + to_string(quantity) +
        " is recognized by the builder, but runtime transmission mapping is not implemented yet");
    }

    return tl::make_unexpected(std::string(e.what()));
  }
}

DefaultJointMapBuilder & DefaultJointMapBuilder::with_urdf(const urdf::Model & urdf_model)
{
  for (const auto & [name, joint] : urdf_model.joints_) {
    if (!joint->mimic)
      continue;

    mimic_joints_[name] = joint->mimic;
  }

  return *this;
}

DefaultJointMapBuilder & DefaultJointMapBuilder::with_transmissions(const std::string & urdf_string, rclcpp::Logger logger)
{
  try {
    return with_transmissions_dangerous(urdf_string);
  } catch (const std::runtime_error & e) {
    RCLCPP_WARN(logger, "Error trying to add transmissions to DefaultJointMapBuilder: %s", e.what());
    return *this;
  }
}

DefaultJointMapBuilder & DefaultJointMapBuilder::with_transmissions_dangerous(const std::string & urdf_string)
{
  tinyxml2::XMLDocument doc;
  doc.Parse(urdf_string.c_str());

  if (!doc.Parse(urdf_string.c_str()) && doc.Error())
    throw std::runtime_error(std::string("invalid URDF passed in to robot parser: ") + doc.ErrorStr());
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

  while (ros2_control_it)
  {
    const auto * ros2_control_child_it = ros2_control_it->FirstChildElement();

    while (ros2_control_child_it) {
      if (std::string(kTransmissionTag) == ros2_control_child_it->Name()) {
        auto transmission = parse_transmission_from_xml(ros2_control_child_it);
        transmissions_.push_back(transmission);

        const auto model_id = transmission_analysis_.add_model(std::make_unique<Ros2ControlTransmissionModel>());

        // TODO: Should we not just build std::vector<JointId> instead? We should have access to a TransmissionAnalysis object at this point
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

        transmission_analysis_.add_transmission(
          model_id,
          span<const std::string>(input_names),
          span<const std::string>(output_names),
          transmission.name);
      }

      ros2_control_child_it = ros2_control_child_it->NextSiblingElement();
    }

    ros2_control_it = ros2_control_it->NextSiblingElement(kROS2ControlTag);
  }

  return *this;
}

DefaultJointMapBuilder & DefaultJointMapBuilder::with_transmission_model(std::unique_ptr<TransmissionModel> model)
{
  transmission_analysis_.add_model(std::move(model));
  return *this;
}

const TransmissionAnalysis & DefaultJointMapBuilder::get_transmission_analysis() const noexcept
{
  return transmission_analysis_;
}

std::string DefaultJointMapBuilder::get_text_for_element(
  const tinyxml2::XMLElement * element_it, const std::string &)
{
  const auto get_text_output = element_it->GetText();
  if (!get_text_output)
    return "";
  return get_text_output;
}

std::string DefaultJointMapBuilder::get_attribute_value(
  const tinyxml2::XMLElement * element_it, const char * attribute_name, std::string tag_name)
{
  const tinyxml2::XMLAttribute * attr = element_it->FindAttribute(attribute_name);
  if (!attr)
    throw std::runtime_error(std::string("no attribute ") + attribute_name + " in " + tag_name + " tag");
  return element_it->Attribute(attribute_name);
}

std::string DefaultJointMapBuilder::get_attribute_value(
  const tinyxml2::XMLElement * element_it, const char * attribute_name, const char * tag_name)
{
  return get_attribute_value(element_it, attribute_name, std::string(tag_name));
}

hardware_interface::JointInfo DefaultJointMapBuilder::parse_transmission_joint_from_xml(
  const tinyxml2::XMLElement * element_it)
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

hardware_interface::ActuatorInfo DefaultJointMapBuilder::parse_transmission_actuator_from_xml(
  const tinyxml2::XMLElement * element_it)
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

static double stod(const std::string & s)
{
  if (const auto result = impl::stod(s))
  {
    return *result;
  }
  throw std::invalid_argument("Failed converting string to real number");
}

double DefaultJointMapBuilder::get_parameter_value_or(
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

std::unordered_map<std::string, std::string> DefaultJointMapBuilder::parse_parameters_from_xml(
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

hardware_interface::TransmissionInfo DefaultJointMapBuilder::parse_transmission_from_xml(
  const tinyxml2::XMLElement * transmission_it)
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

} // arm_kinematics
