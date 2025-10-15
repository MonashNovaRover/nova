//
// Created by Bailey Chessum on 15/10/2025.
//

#include <arm_kinematics/joint_map_builder.hpp>

namespace arm_kinematics {


JointMap JointMapBuilder::build(const std::vector<std::string> & input_names,
                                const std::vector<std::string> & output_names) const {
  return{input_names, output_names, mimic_joints};
}

JointMapBuilder & JointMapBuilder::with_urdf(const urdf::Model & urdf_model) {
  // Find all mimic joints
  // TODO: guard against cyclic mimic joints
  for (const auto & [name, joint] : urdf_model.joints_) {
    if (!joint->mimic)
      continue;

    mimic_joints[name] = joint->mimic;
  }

  return *this;
}

JointMapBuilder & JointMapBuilder::with_transmissions(const std::string & urdf_string) {
  tinyxml2::XMLDocument doc;
  doc.Parse(urdf_string.c_str());

  if (!doc.Parse(urdf_string.c_str()) && doc.Error())
    throw std::runtime_error(std::string("invalid URDF passed in to robot parser: ") + doc.ErrorStr());
  if (doc.Error())
    throw std::runtime_error(std::string("invalid URDF passed in to robot parser: ") + doc.ErrorStr());

  // Find robot or sdf tag
  const tinyxml2::XMLElement * robot_it = doc.RootElement();
  const tinyxml2::XMLElement * ros2_control_it;

  if (std::string(kRobotTag) == robot_it->Name())
  {
    ros2_control_it = robot_it->FirstChildElement(kROS2ControlTag);
  }
  else if (std::string(kSDFTag) == robot_it->Name())
  {
    // find model tag in sdf tag
    const tinyxml2::XMLElement * model_it = robot_it->FirstChildElement(kModelTag);
    ros2_control_it = model_it->FirstChildElement(kROS2ControlTag);
  }
  else
    throw std::runtime_error(
      "the robot tag is not root element in URDF or sdf tag is not root element in SDF");

  if (!ros2_control_it)
    throw std::runtime_error(std::string("no ") + kROS2ControlTag + " tag");

  // Iterate over every ros2_control tag
  while (ros2_control_it)
  {
    const auto * ros2_control_child_it = ros2_control_it->FirstChildElement();

    // Iterate over every child of the ros2_control tag
    while (ros2_control_child_it) {
      if (std::string(kTransmissionTag) == ros2_control_child_it->Name())
        transmissions_.push_back(parse_transmission_from_xml(ros2_control_child_it));

      ros2_control_child_it = ros2_control_child_it->NextSiblingElement();
    }

    ros2_control_it = ros2_control_it->NextSiblingElement(kROS2ControlTag);
  }

  return *this;
}

std::string JointMapBuilder::get_text_for_element(const tinyxml2::XMLElement * element_it, const std::string & tag_name) {
  const auto get_text_output = element_it->GetText();
  if (!get_text_output)
  {
    // TODO: Log this error
    // "text not specified in the " << tag_name << " tag" << std::endl;
    return "";
  }
  return get_text_output;
}

std::string JointMapBuilder::get_attribute_value(const tinyxml2::XMLElement * element_it, const char * attribute_name,
                                                 std::string tag_name) {
  const tinyxml2::XMLAttribute * attr;
  attr = element_it->FindAttribute(attribute_name);
  if (!attr)
    throw std::runtime_error(std::string("no attribute ") + attribute_name + " in " + tag_name + " tag");
  return element_it->Attribute(attribute_name);
}

std::string JointMapBuilder::get_attribute_value(const tinyxml2::XMLElement * element_it, const char * attribute_name,
                                                 const char * tag_name) {
  return get_attribute_value(element_it, attribute_name, std::string(tag_name));
}

hardware_interface::JointInfo
JointMapBuilder::parse_transmission_joint_from_xml(const tinyxml2::XMLElement * element_it) {
  hardware_interface::JointInfo joint_info;
  joint_info.name = get_attribute_value(element_it, kNameAttribute, element_it->Name());
  joint_info.role = get_attribute_value(element_it, kRoleAttribute, element_it->Name());
  joint_info.mechanical_reduction =
    get_parameter_value_or(element_it->FirstChildElement(), kReductionAttribute, 1.0);
  joint_info.offset =
    get_parameter_value_or(element_it->FirstChildElement(), kOffsetAttribute, 0.0);
  return joint_info;
}

hardware_interface::ActuatorInfo
JointMapBuilder::parse_transmission_actuator_from_xml(const tinyxml2::XMLElement * element_it) {
  hardware_interface::ActuatorInfo actuator_info;
  actuator_info.name = get_attribute_value(element_it, kNameAttribute, element_it->Name());
  actuator_info.role = get_attribute_value(element_it, kRoleAttribute, element_it->Name());
  actuator_info.mechanical_reduction =
    get_parameter_value_or(element_it->FirstChildElement(), kReductionAttribute, 1.0);
  actuator_info.offset =
    get_parameter_value_or(element_it->FirstChildElement(), kOffsetAttribute, 0.0);
  return actuator_info;
}

double JointMapBuilder::get_parameter_value_or(const tinyxml2::XMLElement * params_it, const char * parameter_name,
                                               const double default_value) {
  while (params_it)
  {
    try
    {
      // Fill the map with parameters
      const auto tag_name = params_it->Name();
      if (strcmp(tag_name, parameter_name) == 0)
      {
        const auto tag_text = params_it->GetText();
        if (tag_text)
        {
          return hardware_interface::stod(tag_text);
        }
      }
    }
    catch (const std::exception & e)
    {
      return default_value;
    }

    params_it = params_it->NextSiblingElement();
  }

  return default_value;
}

std::unordered_map<std::string, std::string>
JointMapBuilder::parse_parameters_from_xml(const tinyxml2::XMLElement * params_it) {
  std::unordered_map<std::string, std::string> parameters;
  const tinyxml2::XMLAttribute * attr;

  while (params_it)
  {
    // Fill the map with parameters
    attr = params_it->FindAttribute(kNameAttribute);
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

hardware_interface::TransmissionInfo
JointMapBuilder::parse_transmission_from_xml(const tinyxml2::XMLElement * transmission_it) {
  hardware_interface::TransmissionInfo transmission;

  // Find name, type and class of a transmission
  transmission.name = get_attribute_value(transmission_it, kNameAttribute, transmission_it->Name());
  const auto * type_it = transmission_it->FirstChildElement(kPluginNameTag);
  if (!type_it)
  {
    throw std::runtime_error("Missing <plugin> tag of <transmission> element in your URDF.");
  }
  transmission.type = get_text_for_element(type_it, kPluginNameTag);

  // Parse joints
  const auto * joint_it = transmission_it->FirstChildElement(kJointTag);
  while (joint_it)
  {
    transmission.joints.push_back(parse_transmission_joint_from_xml(joint_it));
    joint_it = joint_it->NextSiblingElement(kJointTag);
  }

  // Parse actuators
  const auto * actuator_it = transmission_it->FirstChildElement(kActuatorTag);
  while (actuator_it)
  {
    transmission.actuators.push_back(parse_transmission_actuator_from_xml(actuator_it));
    actuator_it = actuator_it->NextSiblingElement(kActuatorTag);
  }

  // Parse parameters
  const auto * params_it = transmission_it->FirstChildElement(kParamTag);
  if (params_it)
  {
    transmission.parameters = parse_parameters_from_xml(params_it);
  }

  return transmission;
}
} // arm_kinematics