//
// Created by Bailey Chessum on 25/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis_import.hpp"

#include <rclcpp/logging.hpp>

#include <hardware_interface/component_parser.hpp>
#include <hardware_interface/types/hardware_interface_type_values.hpp>
#include <transmission_interface/handle.hpp>

#include <memory>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace arm_kinematics {

namespace {

const char * get_interface_name_for_quantity(const JointQuantity quantity)
{
  switch (quantity) {
    case JointQuantity::Position:
      return hardware_interface::HW_IF_POSITION;
    case JointQuantity::Velocity:
      return hardware_interface::HW_IF_VELOCITY;
  }

  throw std::invalid_argument("Unsupported JointQuantity for ros2_control transmission wrapper");
}

void configure_ros2_control_transmission_for_quantity(
  const std::shared_ptr<transmission_interface::Transmission> & transmission,
  const hardware_interface::TransmissionInfo & transmission_info,
  const JointQuantity quantity,
  std::vector<double> & actuator_values,
  std::vector<double> & joint_values,
  std::vector<transmission_interface::ActuatorHandle> & actuator_handles,
  std::vector<transmission_interface::JointHandle> & joint_handles)
{
  if (transmission->num_actuators() != transmission_info.actuators.size()) {
    throw std::runtime_error(
      "ros2_control transmission '" + transmission_info.name +
      "' reported an actuator count inconsistent with its TransmissionInfo");
  }
  if (transmission->num_joints() != transmission_info.joints.size()) {
    throw std::runtime_error(
      "ros2_control transmission '" + transmission_info.name +
      "' reported a joint count inconsistent with its TransmissionInfo");
  }

  actuator_values.assign(transmission_info.actuators.size(), 0.0);
  joint_values.assign(transmission_info.joints.size(), 0.0);

  const auto * interface_name = get_interface_name_for_quantity(quantity);

  actuator_handles.clear();
  actuator_handles.reserve(transmission_info.actuators.size());
  for (size_t i = 0; i < transmission_info.actuators.size(); ++i) {
    actuator_handles.emplace_back(
      transmission_info.actuators[i].name,
      interface_name,
      &actuator_values[i]);
  }

  joint_handles.clear();
  joint_handles.reserve(transmission_info.joints.size());
  for (size_t i = 0; i < transmission_info.joints.size(); ++i) {
    joint_handles.emplace_back(
      transmission_info.joints[i].name,
      interface_name,
      &joint_values[i]);
  }

  transmission->configure(joint_handles, actuator_handles);
}

class Ros2ControlPluginTransmissionCompute final : public ComputeTransmission {
public:
  Ros2ControlPluginTransmissionCompute(
    std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader,
    hardware_interface::TransmissionInfo transmission_info,
    const JointQuantity quantity,
    const PropagationDirection direction)
    : plugin_loader_(std::move(plugin_loader)),
      transmission_info_(std::move(transmission_info)),
      quantity_(quantity),
      direction_(direction),
      actuator_values_(transmission_info_.actuators.size(), 0.0),
      joint_values_(transmission_info_.joints.size(), 0.0)
  {
    if (!plugin_loader_) {
      throw std::invalid_argument("Ros2ControlPluginTransmissionCompute received a null plugin loader");
    }

    transmission_ = plugin_loader_->load(transmission_info_);
    configure_ros2_control_transmission_for_quantity(
      transmission_,
      transmission_info_,
      quantity_,
      actuator_values_,
      joint_values_,
      actuator_handles_,
      joint_handles_);
  }

  void compute(
    const span<const float> inputs,
    const span<float> outputs,
    const span<float>) const override
  {
    if (direction_ == PropagationDirection::Forward) {
      if (inputs.size() != actuator_values_.size() || outputs.size() != joint_values_.size()) {
        throw std::invalid_argument(
          "Ros2ControlPluginTransmissionCompute received a forward request with mismatched input/output sizes");
      }

      for (size_t i = 0; i < actuator_values_.size(); ++i) {
        actuator_values_[i] = inputs[i];
      }

      transmission_->actuator_to_joint();

      for (size_t i = 0; i < joint_values_.size(); ++i) {
        outputs[i] = static_cast<float>(joint_values_[i]);
      }
      return;
    }

    if (inputs.size() != joint_values_.size() || outputs.size() != actuator_values_.size()) {
      throw std::invalid_argument(
        "Ros2ControlPluginTransmissionCompute received a reverse request with mismatched input/output sizes");
    }

    for (size_t i = 0; i < joint_values_.size(); ++i) {
      joint_values_[i] = inputs[i];
    }

    transmission_->joint_to_actuator();

    for (size_t i = 0; i < actuator_values_.size(); ++i) {
      outputs[i] = static_cast<float>(actuator_values_[i]);
    }
  }

  [[nodiscard]] size_t scratch_size() const noexcept override
  {
    return 0;
  }

  [[nodiscard]] std::unique_ptr<ComputeTransmission> clone() const override
  {
    return std::make_unique<Ros2ControlPluginTransmissionCompute>(
      plugin_loader_,
      transmission_info_,
      quantity_,
      direction_);
  }

private:
  std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader_{};
  hardware_interface::TransmissionInfo transmission_info_{};
  JointQuantity quantity_{JointQuantity::Position};
  PropagationDirection direction_{PropagationDirection::Forward};

  std::shared_ptr<transmission_interface::Transmission> transmission_{};
  mutable std::vector<double> actuator_values_{};
  mutable std::vector<double> joint_values_{};
  std::vector<transmission_interface::ActuatorHandle> actuator_handles_{};
  std::vector<transmission_interface::JointHandle> joint_handles_{};
};

class Ros2ControlPluginTransmissionModel final : public TransmissionModel {
public:
  Ros2ControlPluginTransmissionModel(
    hardware_interface::TransmissionInfo transmission_info,
    std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader)
    : transmission_info_(std::move(transmission_info)),
      plugin_loader_(std::move(plugin_loader))
  {
    validate();
  }

  [[nodiscard]] std::unique_ptr<TransmissionModel> clone() const override
  {
    return std::unique_ptr<TransmissionModel>(new Ros2ControlPluginTransmissionModel(
      transmission_info_,
      plugin_loader_,
      is_loadable_,
      supports_position_,
      supports_velocity_,
      actuator_count_,
      joint_count_));
  }

  [[nodiscard]] bool can_build(
    const JointQuantity quantity,
    const PropagationDirection) const noexcept override
  {
    switch (quantity) {
      case JointQuantity::Position:
        return is_loadable_ && supports_position_;
      case JointQuantity::Velocity:
        return is_loadable_ && supports_velocity_;
    }

    return false;
  }

  [[nodiscard]] std::unique_ptr<const ComputeTransmission> build(
    const JointQuantity quantity,
    const PropagationDirection direction,
    const span<const JointId> input_joint_ids,
    const span<const JointId> output_joint_ids) const override
  {
    if (!can_build(quantity, direction)) {
      throw std::invalid_argument(
        "Ros2ControlPluginTransmissionModel::build() called for an unsupported quantity or unloaded plugin type");
    }

    const auto expected_input_count =
      direction == PropagationDirection::Forward ? actuator_count_ : joint_count_;
    const auto expected_output_count =
      direction == PropagationDirection::Forward ? joint_count_ : actuator_count_;

    if (input_joint_ids.size() != expected_input_count || output_joint_ids.size() != expected_output_count) {
      throw std::invalid_argument(
        "Ros2ControlPluginTransmissionModel::build() received input/output counts inconsistent with the transmission");
    }

    return std::make_unique<Ros2ControlPluginTransmissionCompute>(
      plugin_loader_,
      transmission_info_,
      quantity,
      direction);
  }

private:
  Ros2ControlPluginTransmissionModel(
    hardware_interface::TransmissionInfo transmission_info,
    std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader,
    const bool is_loadable,
    const bool supports_position,
    const bool supports_velocity,
    const size_t actuator_count,
    const size_t joint_count)
    : transmission_info_(std::move(transmission_info)),
      plugin_loader_(std::move(plugin_loader)),
      is_loadable_(is_loadable),
      supports_position_(supports_position),
      supports_velocity_(supports_velocity),
      actuator_count_(actuator_count),
      joint_count_(joint_count)
  {
  }

  [[nodiscard]] bool supports_quantity(const JointQuantity quantity) const
  {
    try {
      auto transmission = plugin_loader_->load(transmission_info_);
      std::vector<double> actuator_values{};
      std::vector<double> joint_values{};
      std::vector<transmission_interface::ActuatorHandle> actuator_handles{};
      std::vector<transmission_interface::JointHandle> joint_handles{};
      configure_ros2_control_transmission_for_quantity(
        transmission,
        transmission_info_,
        quantity,
        actuator_values,
        joint_values,
        actuator_handles,
        joint_handles);
      actuator_count_ = transmission->num_actuators();
      joint_count_ = transmission->num_joints();
      return true;
    } catch (const std::exception &) {
      return false;
    }
  }

  void validate()
  {
    if (!plugin_loader_) {
      return;
    }

    if (!plugin_loader_->has_plugin_type(transmission_info_.type)) {
      return;
    }

    try {
      auto transmission = plugin_loader_->load(transmission_info_);
      actuator_count_ = transmission->num_actuators();
      joint_count_ = transmission->num_joints();
      if (
        actuator_count_ != transmission_info_.actuators.size() ||
        joint_count_ != transmission_info_.joints.size())
      {
        return;
      }

      supports_position_ = supports_quantity(JointQuantity::Position);
      supports_velocity_ = supports_quantity(JointQuantity::Velocity);
      is_loadable_ = supports_position_ || supports_velocity_;
    } catch (const std::exception &) {
      is_loadable_ = false;
    }
  }

  hardware_interface::TransmissionInfo transmission_info_{};
  std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader_{};
  bool is_loadable_ = false;
  mutable bool supports_position_ = false;
  mutable bool supports_velocity_ = false;
  mutable size_t actuator_count_ = 0;
  mutable size_t joint_count_ = 0;
};

void add_ros2_control_transmission_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const hardware_interface::TransmissionInfo & transmission,
  const std::shared_ptr<const Ros2ControlTransmissionPluginLoader> & plugin_loader)
{
  const auto model_id = transmission_analysis.add_model(
    std::make_unique<Ros2ControlPluginTransmissionModel>(transmission, plugin_loader));

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
  const std::string & urdf_string,
  std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader)
{
  if (!plugin_loader) {
    plugin_loader = std::make_shared<Ros2ControlTransmissionPluginLoader>();
  }

  std::vector<hardware_interface::TransmissionInfo> transmissions{};
  for (auto & hardware_info : hardware_interface::parse_control_resources_from_urdf(urdf_string)) {
    for (auto & transmission : hardware_info.transmissions) {
      add_ros2_control_transmission_to_analysis(transmission_analysis, transmission, plugin_loader);
      transmissions.push_back(std::move(transmission));
    }
  }

  if (transmissions.empty()) {
    throw std::runtime_error("no ros2_control transmissions found");
  }

  return transmissions;
}

std::vector<hardware_interface::TransmissionInfo> add_ros2_control_transmissions_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const std::string & urdf_string,
  std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader,
  rclcpp::Logger logger)
{
  try {
    return add_ros2_control_transmissions_to_analysis_dangerous(
      transmission_analysis,
      urdf_string,
      std::move(plugin_loader));
  } catch (const std::runtime_error & e) {
    RCLCPP_WARN(logger, "Error trying to add transmissions to TransmissionAnalysis: %s", e.what());
    return {};
  }
}

std::vector<hardware_interface::TransmissionInfo> add_ros2_control_transmissions_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const std::string & urdf_string,
  rclcpp::Logger logger)
{
  return add_ros2_control_transmissions_to_analysis(
    transmission_analysis,
    urdf_string,
    std::make_shared<Ros2ControlTransmissionPluginLoader>(),
    logger);
}

void add_mimic_transmissions_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const urdf::Model & urdf_model)
{
  for (const auto & [joint_name, _] : urdf_model.joints_) {
    transmission_analysis.ensure_joint_id(joint_name);
  }

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
