//
// Created by Bailey Chessum on 15/10/2025.
//

#ifndef ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
#define ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP

#include <urdf/model.h>
#include <kdl_parser/kdl_parser.hpp>
#include <rclcpp/logger.hpp>
#include <hardware_interface/component_parser.hpp>
#include <tinyxml2.h>

#include "arm_kinematics/visibility_control.h"
#include "arm_kinematics/joint_map/joint_map.hpp"

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

class ARM_KINEMATICS_PUBLIC JointMapBuilder {
public:
  JointMapBuilder() = default;

  /**
   * Constructs a JointMap using the data from previous with_* calls, that maps inputs with input_names to outputs with
   * output_names.
   *
   * \warning Not real-time safe
   */
  [[nodiscard]] JointMap build(const std::vector<std::string> & input_names,
                               const std::vector<std::string> & output_names) const;

  /**
   * Constructs a JointMap using the data from previous with_* calls, that maps inputs with input_names to each Jnt from
   * a KDL::Chain's JntArray.
   *
   * \warning Not real-time safe
   */
  [[nodiscard]] JointMap build(const std::vector<std::string> & input_names,
                               const KDL::Chain & chain) const;

  /**
   * Uses the given URDF model to add mimic joints.
   *
   * \param urdf_model The urdf::Model to get mimic joints from.
   */
  JointMapBuilder & with_urdf(const urdf::Model & urdf_model);

  /**
   * Manually parses the given URDF string to get transmission definitions to include in the joint map.
   *
   * \note This overload gracefully fails with a log message rather than throwing an exception. No transmissions are
   *       added in the case of an exception occuring.
   *
   * \param urdf_string The URDF as a string, containing a ros2_control definition
   * \param logger the logger to log any caught exceptions to
   */
  JointMapBuilder & with_transmissions(const std::string & urdf_string, rclcpp::Logger logger);

  /**
   * Manually parses the given URDF string to get transmission definitions to include in the joint map.
   *
   * \param urdf_string The URDF as a string, containing a ros2_control definition
   * \throws std::runtime_error for invalid URDFs
   */
  JointMapBuilder & with_transmissions_dangerous(const std::string & urdf_string);

  std::vector<hardware_interface::TransmissionInfo> transmissions_{};
  std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints{};

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
  static std::string get_text_for_element(
    const tinyxml2::XMLElement * element_it, const std::string & tag_name);

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
  static std::string get_attribute_value(
    const tinyxml2::XMLElement * element_it, const char * attribute_name, std::string tag_name);

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
  static std::string get_attribute_value(
    const tinyxml2::XMLElement * element_it, const char * attribute_name, const char * tag_name);

  static hardware_interface::JointInfo parse_transmission_joint_from_xml(const tinyxml2::XMLElement * element_it);

  static hardware_interface::ActuatorInfo parse_transmission_actuator_from_xml(const tinyxml2::XMLElement * element_it);

  /// Gets value of the parameter on an XMLelement.
/**
 * If parameter is not found, returns specified default value
 *
 * \param[in] element_it XMLElement iterator to search for the attribute
 * \param[in] attribute_name attribute name to search for and return value
 * \param[in] default_value When the attribute is not found, this value is returned instead
 * \return attribute value or default
 */
  static double get_parameter_value_or(
    const tinyxml2::XMLElement * params_it, const char * parameter_name, const double default_value);

  /// Search XML snippet from URDF for parameters.
  /**
   * \param[in] params_it pointer to the iterator where parameters info should be found
   * \return key-value map with parameters
   * \throws std::runtime_error if a component attribute or tag is not found
   */
  static std::unordered_map<std::string, std::string> parse_parameters_from_xml(const tinyxml2::XMLElement * params_it);

  /// Search XML snippet from URDF for information about a transmission.
  /**
   * \param[in] transmission_it pointer to the iterator where transmission info should be found
   * \return TransmissionInfo filled with information about transmission
   * \throws std::runtime_error if an attribute or tag is not found
   */
  static hardware_interface::TransmissionInfo parse_transmission_from_xml(const tinyxml2::XMLElement * transmission_it);
};

} // arm_kinematics

#endif //ARM_KINEMATICS_JOINT_MAP_BUILDER_HPP
