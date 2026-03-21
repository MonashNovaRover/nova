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
#include "arm_kinematics/joint_map/transmission_analysis.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"
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

/**
 * Default shared builder backed by the robot model's URDF-derived metadata.
 *
 * Stage 1 behavior:
 *   - gathers mimic-joint definitions from the URDF
 *   - parses ros2_control transmission definitions
 *   - caches a TransmissionAnalysis built from normalized transmission models
 *   - still only succeeds at runtime for AffineJointMap-style requests
 *
 * This type is the default shared implementation, not the only possible implementation. FK plugins may return a
 * different JointMapBuilder when they need backend-specific mapping behavior.
 */
class ARM_KINEMATICS_PUBLIC DefaultJointMapBuilder final : public JointMapBuilder {
public:
  DefaultJointMapBuilder() = default;

  /**
   * Constructs a JointMap that maps input_names to output_names for the requested quantity.
   *
   * Today this succeeds for affine reorder/mimic cases and reports a build-time error for recognized
   * transmission-backed requests, since the runtime transmission compute path is not implemented yet.
   */
  [[nodiscard]] tl::expected<JointMap, std::string> build_expected(
    const std::vector<std::string> & input_names,
    const std::vector<std::string> & output_names,
    JointQuantity quantity) const override;

  /**
   * Uses the given URDF model to add mimic joints.
   *
   * \param urdf_model The urdf::Model to get mimic joints from.
   */
  DefaultJointMapBuilder & with_urdf(const urdf::Model & urdf_model);

  /**
   * Manually parses the given URDF string to get transmission definitions to include in the builder.
   *
   * \note This overload gracefully fails with a log message rather than throwing an exception. No transmissions are
   *       added in the case of an exception occurring.
   *
   * \param urdf_string The URDF as a string, containing a ros2_control definition.
   * \param logger the logger to log any caught exceptions to.
   */
  DefaultJointMapBuilder & with_transmissions(const std::string & urdf_string, rclcpp::Logger logger);

  /**
   * Manually parses the given URDF string to get transmission definitions to include in the builder.
   *
   * \param urdf_string The URDF as a string, containing a ros2_control definition.
   * \throws std::runtime_error for invalid URDFs.
   */
  DefaultJointMapBuilder & with_transmissions_dangerous(const std::string & urdf_string);

  /**
   * Adds a normalized transmission model to the cached transmission analysis.
   */
  DefaultJointMapBuilder & with_transmission_model(std::unique_ptr<TransmissionModel> model);

  [[nodiscard]] const TransmissionAnalysis & get_transmission_analysis() const noexcept;

private:
  // ros2_control transmission XML parsers
  // source from ros2_control hardware_interface component parser

  /// Gets value of the text between tags.
  static std::string get_text_for_element(
    const tinyxml2::XMLElement * element_it, const std::string & tag_name);

  /// Gets value of the attribute on an XMLElement.
  static std::string get_attribute_value(
    const tinyxml2::XMLElement * element_it, const char * attribute_name, std::string tag_name);

  /// Gets value of the attribute on an XMLElement.
  static std::string get_attribute_value(
    const tinyxml2::XMLElement * element_it, const char * attribute_name, const char * tag_name);

  static hardware_interface::JointInfo parse_transmission_joint_from_xml(const tinyxml2::XMLElement * element_it);
  static hardware_interface::ActuatorInfo parse_transmission_actuator_from_xml(const tinyxml2::XMLElement * element_it);

  /// Gets value of the parameter on an XMLElement, or a default if absent.
  static double get_parameter_value_or(
    const tinyxml2::XMLElement * params_it, const char * parameter_name, double default_value);

  /// Search XML snippet from URDF for parameters.
  static std::unordered_map<std::string, std::string> parse_parameters_from_xml(
    const tinyxml2::XMLElement * params_it);

  /// Search XML snippet from URDF for information about a transmission.
  static hardware_interface::TransmissionInfo parse_transmission_from_xml(
    const tinyxml2::XMLElement * transmission_it);

  /// Parsed transmission metadata from ros2_control XML. Parsed for future use, but not consumed in Stage 1 build().
  std::vector<hardware_interface::TransmissionInfo> transmissions_{};
  /// Mimic joints gathered from the URDF.
  std::map<std::string, std::shared_ptr<urdf::JointMimic>> mimic_joints_{};
  /// Cached structural transmission analysis derived from registered transmission models.
  TransmissionAnalysis transmission_analysis_{};
};

} // arm_kinematics

#endif //ARM_KINEMATICS_DEFAULT_JOINT_MAP_BUILDER_HPP
