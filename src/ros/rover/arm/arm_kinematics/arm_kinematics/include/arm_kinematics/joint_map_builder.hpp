//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
#define ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP

#include <cstddef>
#include <urdf/model.h>
#include <kdl_parser/kdl_parser.hpp>
#include <kdl/jntarray.hpp>
#include <rclcpp/logger.hpp>
#include <variant>

#include <rclcpp/logging.hpp>
#include <hardware_interface/component_parser.hpp>
#include "hardware_interface/lexical_casts.hpp"

#include <tinyxml2.h>

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

class JointMapBuilder {
  JointMapBuilder() = default;

  JointMapBuilder & with_urdf(const urdf::Model urdf_model) {

    // Find all mimic joints
    // TODO: guard against cyclic mimic joints
    std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints{};
    for (const auto & [name, joint] : urdf_model.joints_) {
      if (!joint->mimic)
        continue;

      mimic_joints[name] = joint->mimic;
    }

    return *this;
  }

  /**
   * Manually parses the given URDF string to get transmission definitions to include in the joint map.
   *
   * \param urdf_string The URDF as a string, containing a ros2_control definition
   * \throws std::runtime_error for invalid URDFs
   */
  JointMapBuilder & with_transmissions(const std::string & urdf_string) {
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

public:
  std::vector<hardware_interface::TransmissionInfo> transmissions_{};
  std::vector<hardware_interface::MimicJoint> mimics_{};

private:

  // ros2_control transmission XML parsers
  // source from https://github.com/ros-controls/ros2_control/blob/896de9646ac73780a314f900fe72a11f2666fbd2/hardware_interface/src/component_parser.cpp#L562

  /// Gets value of the text between tags.
  /**
   * \param[in] element_it XMLElement iterator to search for the text.
   * \param[in] tag_name parent tag name where text is searched for (used for error output)
   * \return text of for the tag
   * \throws std::runtime_error if text is not found
   */
  std::string get_text_for_element(
    const tinyxml2::XMLElement * element_it, const std::string & tag_name)
  {
    const auto get_text_output = element_it->GetText();
    if (!get_text_output)
    {
      // TODO: Log this error
      // "text not specified in the " << tag_name << " tag" << std::endl;
      return "";
    }
    return get_text_output;
  }

  /// Gets value of the attribute on an XMLelement.
  /**
   * If attribute is not found throws an error.
   *
   * \param[in] element_it XMLElement iterator to search for the attribute
   * \param[in] attribute_name attribute name to search for and return value
   * \param[in] tag_name parent tag name where attribute is searched for (used for error output)
   * \return attribute value
   * \throws std::runtime_error if attribute is not found
   */
  std::string get_attribute_value(
    const tinyxml2::XMLElement * element_it, const char * attribute_name, std::string tag_name)
  {
    const tinyxml2::XMLAttribute * attr;
    attr = element_it->FindAttribute(attribute_name);
    if (!attr)
      throw std::runtime_error(std::string("no attribute ") + attribute_name + " in " + tag_name + " tag");
    return element_it->Attribute(attribute_name);
  }

  /// Gets value of the attribute on an XMLelement.
  /**
   * If attribute is not found throws an error.
   *
   * \param[in] element_it XMLElement iterator to search for the attribute
   * \param[in] attribute_name attribute name to search for and return value
   * \param[in] tag_name parent tag name where attribute is searched for (used for error output)
   * \return attribute value
   * \throws std::runtime_error if attribute is not found
   */
  std::string get_attribute_value(
    const tinyxml2::XMLElement * element_it, const char * attribute_name, const char * tag_name)
  {
    return get_attribute_value(element_it, attribute_name, std::string(tag_name));
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

  /// Gets value of the parameter on an XMLelement.
/**
 * If parameter is not found, returns specified default value
 *
 * \param[in] element_it XMLElement iterator to search for the attribute
 * \param[in] attribute_name attribute name to search for and return value
 * \param[in] default_value When the attribute is not found, this value is returned instead
 * \return attribute value or default
 */
  double get_parameter_value_or(
    const tinyxml2::XMLElement * params_it, const char * parameter_name, const double default_value)
  {
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

  /// Search XML snippet from URDF for parameters.
  /**
   * \param[in] params_it pointer to the iterator where parameters info should be found
   * \return key-value map with parameters
   * \throws std::runtime_error if a component attribute or tag is not found
   */
  std::unordered_map<std::string, std::string> parse_parameters_from_xml(const tinyxml2::XMLElement * params_it)
  {
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

  /// Search XML snippet from URDF for information about a transmission.
  /**
   * \param[in] transmission_it pointer to the iterator where transmission info should be found
   * \return TransmissionInfo filled with information about transmission
   * \throws std::runtime_error if an attribute or tag is not found
   */
  hardware_interface::TransmissionInfo parse_transmission_from_xml(const tinyxml2::XMLElement * transmission_it)
  {
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



};

} // arm_kinematics

#endif //ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
