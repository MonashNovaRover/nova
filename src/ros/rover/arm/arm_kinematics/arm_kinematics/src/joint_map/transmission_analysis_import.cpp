//
// Created by Bailey Chessum on 25/03/2026.
//

#include "arm_kinematics/joint_map/transmission_analysis_import.hpp"

#include <rclcpp/logging.hpp>

#include <hardware_interface/component_parser.hpp>
#include <transmission_interface/handle.hpp>

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "arm_kinematics/joint_map/state_interface_definition.hpp"
#include "arm_kinematics/joint_map/transmission_model.hpp"
#include "arm_kinematics/utilities/interface_id.hpp"

namespace arm_kinematics {

using StateInterfaceId = TransmissionAnalysis::StateInterfaceId;

namespace {

// ===========================================================================
// ros2_control plugin wrapper helpers
//
// Each ros2_control `<transmission>` XML element is wrapped in a single
// `Ros2ControlPluginCore` (heap-allocated, shared via shared_ptr). The core
// loads the plugin once at construction and probes which
// `(interface_id × direction)` combinations the plugin actually supports
// (using fresh dummy `Transmission` instances + try/catch around configure
// and the per-direction propagation calls).
//
// Each supported combo is then registered with the analysis as its own
// `TransmissionInstance`, backed by a tiny `Ros2ControlPluginTransmissionInstanceModel`
// that holds a shared_ptr to the core and remembers its own
// (interface_id, direction). At `build()` time the instance model asks the
// core to mint a freshly-configured `Transmission` and hands it to a
// `Ros2ControlPluginTransmissionCompute` that just shovels values back and
// forth across the plugin handles.
//
// This avoids the legacy design's per-Compute plugin reload, and lets the
// planner select the right (direction × interface) without the model having
// to introspect SIDs.
//
// **Granularity note.** `TransmissionAnalysis` is a general-purpose structure
// for analyzing relationships between joint properties; it is NOT designed
// around ros2_control specifically. The per-(interface × direction) expansion
// here is the importer adapting to the analysis's natural shape (one
// `TransmissionInstance` = one directed input→output edge), not the analysis
// bending to match the ros2_control plugin model.
// ===========================================================================

/// Bundles a freshly loaded + freshly configured `Transmission` instance
/// with the value vectors and handles it was bound to.
///
/// **Critical lifetime invariant:** the `JointHandle` / `ActuatorHandle`
/// objects store raw `double *` pointers into `actuator_values` and
/// `joint_values`. The vectors **must not be relocated** after the handles
/// are constructed. Callers therefore allocate this struct on the heap (via
/// `unique_ptr`) and never copy or move it.
struct ConfiguredTransmission {
  std::shared_ptr<transmission_interface::Transmission> transmission{};
  std::vector<double> actuator_values{};
  std::vector<double> joint_values{};
  std::vector<transmission_interface::ActuatorHandle> actuator_handles{};
  std::vector<transmission_interface::JointHandle> joint_handles{};

  ConfiguredTransmission() = default;
  ConfiguredTransmission(const ConfiguredTransmission &) = delete;
  ConfiguredTransmission & operator=(const ConfiguredTransmission &) = delete;
  ConfiguredTransmission(ConfiguredTransmission &&) = delete;
  ConfiguredTransmission & operator=(ConfiguredTransmission &&) = delete;
};

/// One supported `(interface_id × direction)` combo for a single
/// ros2_control transmission XML element.
struct SupportedCombo {
  InterfaceId interface_id{};
  bool forward = false;
  bool inverse = false;
};

/// Loads a fresh `Transmission` instance via the plugin loader and configures
/// it against fresh handles bound to the given interface name. The handles
/// point into the returned struct's own value vectors, so the struct must
/// stay heap-pinned for the lifetime of the handles.
std::unique_ptr<ConfiguredTransmission> make_configured_transmission(
  const Ros2ControlTransmissionPluginLoader & plugin_loader,
  const hardware_interface::TransmissionInfo & info,
  const std::string & interface_name)
{
  auto out = std::make_unique<ConfiguredTransmission>();
  out->transmission = plugin_loader.load(info);
  if (!out->transmission) {
    throw std::runtime_error(
      "ros2_control transmission '" + info.name + "': plugin loader returned a null transmission");
  }

  if (out->transmission->num_actuators() != info.actuators.size()) {
    throw std::runtime_error(
      "ros2_control transmission '" + info.name +
      "' reported an actuator count inconsistent with its TransmissionInfo");
  }
  if (out->transmission->num_joints() != info.joints.size()) {
    throw std::runtime_error(
      "ros2_control transmission '" + info.name +
      "' reported a joint count inconsistent with its TransmissionInfo");
  }

  out->actuator_values.assign(info.actuators.size(), 0.0);
  out->joint_values.assign(info.joints.size(), 0.0);

  out->actuator_handles.reserve(info.actuators.size());
  for (size_t i = 0; i < info.actuators.size(); ++i) {
    out->actuator_handles.emplace_back(
      info.actuators[i].name,
      interface_name,
      &out->actuator_values[i]);
  }

  out->joint_handles.reserve(info.joints.size());
  for (size_t i = 0; i < info.joints.size(); ++i) {
    out->joint_handles.emplace_back(
      info.joints[i].name,
      interface_name,
      &out->joint_values[i]);
  }

  out->transmission->configure(out->joint_handles, out->actuator_handles);
  return out;
}

/// Heavy state for one ros2_control `<transmission>` element. Probes once at
/// construction; shared by reference between every per-combo
/// `TransmissionModel` registered for this element.
struct Ros2ControlPluginCore {
  std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader{};
  hardware_interface::TransmissionInfo info{};
  size_t actuator_count = 0;
  size_t joint_count = 0;
  std::vector<SupportedCombo> supported_combos{};

  Ros2ControlPluginCore(
    hardware_interface::TransmissionInfo transmission_info,
    std::shared_ptr<const Ros2ControlTransmissionPluginLoader> loader,
    span<const InterfaceId> probe_interfaces)
    : plugin_loader(std::move(loader)),
      info(std::move(transmission_info))
  {
    if (!plugin_loader) {
      return;
    }
    if (!plugin_loader->has_plugin_type(info.type)) {
      return;
    }
    probe(probe_interfaces);
  }

  /// Mints a freshly loaded + freshly configured Transmission for the given
  /// interface name. Throws on plugin/configure failure.
  [[nodiscard]] std::unique_ptr<ConfiguredTransmission> make_configured(
    const std::string & interface_name) const
  {
    return make_configured_transmission(*plugin_loader, info, interface_name);
  }

private:
  void probe(const span<const InterfaceId> probe_interfaces)
  {
    // Establish actuator/joint counts from one plain load. If the very first
    // load fails or counts mismatch, the plugin is unusable for this info.
    try {
      const auto probe_load = plugin_loader->load(info);
      if (!probe_load) {
        return;
      }
      actuator_count = probe_load->num_actuators();
      joint_count = probe_load->num_joints();
      if (
        actuator_count != info.actuators.size() ||
        joint_count != info.joints.size())
      {
        actuator_count = 0;
        joint_count = 0;
        return;
      }
    } catch (const std::exception &) {
      return;
    }

    for (const auto & interface_id : probe_interfaces) {
      const std::string & interface_name = interface_id.name;
      SupportedCombo combo{interface_id, false, false};

      std::unique_ptr<ConfiguredTransmission> configured{};
      try {
        configured = make_configured_transmission(*plugin_loader, info, interface_name);
      } catch (const std::exception &) {
        // Plugin doesn't support this interface_id at configure time. Skip
        // both directions.
        continue;
      }

      try {
        configured->transmission->actuator_to_joint();
        combo.forward = true;
      } catch (const std::exception &) {
        // Plugin loaded + configured but doesn't implement this direction
        // for this interface. Leave `forward` false.
      }

      try {
        configured->transmission->joint_to_actuator();
        combo.inverse = true;
      } catch (const std::exception &) {
      }

      if (combo.forward || combo.inverse) {
        supported_combos.push_back(std::move(combo));
      }
    }
  }
};

/// Runtime adapter that copies inputs into the configured value vectors,
/// asks the plugin to propagate, and copies the results back out.
class Ros2ControlPluginTransmissionCompute final : public ComputeTransmission {
public:
  Ros2ControlPluginTransmissionCompute(
    std::shared_ptr<const Ros2ControlPluginCore> core,
    std::string interface_name,
    const bool is_forward,
    std::unique_ptr<ConfiguredTransmission> configured)
    : core_(std::move(core)),
      interface_name_(std::move(interface_name)),
      is_forward_(is_forward),
      configured_(std::move(configured))
  {
    if (!configured_) {
      throw std::invalid_argument(
        "Ros2ControlPluginTransmissionCompute received a null ConfiguredTransmission");
    }
  }

  void compute(
    const span<const double> inputs,
    const span<double> outputs,
    const span<double>) const override
  {
    auto & av = configured_->actuator_values;
    auto & jv = configured_->joint_values;

    if (is_forward_) {
      if (inputs.size() != av.size() || outputs.size() != jv.size()) {
        throw std::invalid_argument(
          "Ros2ControlPluginTransmissionCompute received a forward request with mismatched input/output sizes");
      }
      for (size_t i = 0; i < av.size(); ++i) {
        av[i] = inputs[i];
      }
      configured_->transmission->actuator_to_joint();
      for (size_t i = 0; i < jv.size(); ++i) {
        outputs[i] = jv[i];
      }
      return;
    }

    if (inputs.size() != jv.size() || outputs.size() != av.size()) {
      throw std::invalid_argument(
        "Ros2ControlPluginTransmissionCompute received a reverse request with mismatched input/output sizes");
    }
    for (size_t i = 0; i < jv.size(); ++i) {
      jv[i] = inputs[i];
    }
    configured_->transmission->joint_to_actuator();
    for (size_t i = 0; i < av.size(); ++i) {
      outputs[i] = av[i];
    }
  }

  [[nodiscard]] size_t scratch_size() const noexcept override
  {
    return 0;
  }

  [[nodiscard]] std::unique_ptr<ComputeTransmission> clone() const override
  {
    // Each clone needs its own freshly-configured plugin instance + value
    // vectors so the underlying handles point at independent storage.
    return std::make_unique<Ros2ControlPluginTransmissionCompute>(
      core_,
      interface_name_,
      is_forward_,
      core_->make_configured(interface_name_));
  }

private:
  std::shared_ptr<const Ros2ControlPluginCore> core_{};
  std::string interface_name_{};
  bool is_forward_ = true;
  std::unique_ptr<ConfiguredTransmission> configured_{};
};

/// One `TransmissionModel` per (interface_id × direction) combo, sharing the
/// same backing `Ros2ControlPluginCore`. Tiny — all heavy state lives in the
/// shared core. Direction and interface_id are baked in at construction so
/// `build()` doesn't need to introspect input/output SIDs to figure them out.
class Ros2ControlPluginTransmissionInstanceModel final : public TransmissionModel {
public:
  Ros2ControlPluginTransmissionInstanceModel(
    std::shared_ptr<const Ros2ControlPluginCore> core,
    InterfaceId interface_id,
    const bool is_forward)
    : core_(std::move(core)),
      interface_id_(std::move(interface_id)),
      is_forward_(is_forward)
  {
    if (!core_) {
      throw std::invalid_argument(
        "Ros2ControlPluginTransmissionInstanceModel received a null core");
    }
  }

  [[nodiscard]] std::unique_ptr<TransmissionModel> clone() const override
  {
    return std::make_unique<Ros2ControlPluginTransmissionInstanceModel>(
      core_, interface_id_, is_forward_);
  }

  [[nodiscard]] std::unique_ptr<const ComputeTransmission> build(
    const span<const StateInterfaceId> input_joint_ids,
    const span<const StateInterfaceId> output_joint_ids) const override
  {
    const auto expected_input_count = is_forward_ ? core_->actuator_count : core_->joint_count;
    const auto expected_output_count = is_forward_ ? core_->joint_count : core_->actuator_count;

    if (
      input_joint_ids.size() != expected_input_count ||
      output_joint_ids.size() != expected_output_count)
    {
      throw std::invalid_argument(
        "Ros2ControlPluginTransmissionInstanceModel::build() received input/output counts inconsistent with the transmission");
    }

    return std::make_unique<Ros2ControlPluginTransmissionCompute>(
      core_,
      interface_id_.name,
      is_forward_,
      core_->make_configured(interface_id_.name));
  }

private:
  std::shared_ptr<const Ros2ControlPluginCore> core_{};
  InterfaceId interface_id_{};
  bool is_forward_ = true;
};

void add_ros2_control_transmission_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const hardware_interface::TransmissionInfo & transmission,
  const std::shared_ptr<const Ros2ControlTransmissionPluginLoader> & plugin_loader,
  const span<const InterfaceId> probe_interfaces)
{
  auto core = std::make_shared<Ros2ControlPluginCore>(transmission, plugin_loader, probe_interfaces);
  if (core->supported_combos.empty()) {
    // Plugin failed to load, didn't match the TransmissionInfo, or supported
    // no probed (interface × direction) combos. Skip silently — the
    // logging-wrapper at the import boundary handles user-visible reporting.
    return;
  }

  for (const auto & combo : core->supported_combos) {
    // Resolve actuator/joint state interface ids for this interface_id.
    std::vector<StateInterfaceId> actuator_sids{};
    actuator_sids.reserve(transmission.actuators.size());
    for (const auto & actuator : transmission.actuators) {
      actuator_sids.push_back(transmission_analysis.ensure_state_interface_id(
        NamedStateInterfaceDefinition{actuator.name, combo.interface_id}));
    }

    std::vector<StateInterfaceId> joint_sids{};
    joint_sids.reserve(transmission.joints.size());
    for (const auto & joint : transmission.joints) {
      joint_sids.push_back(transmission_analysis.ensure_state_interface_id(
        NamedStateInterfaceDefinition{joint.name, combo.interface_id}));
    }

    if (combo.forward) {
      const auto fwd_model_id = transmission_analysis.add_model(
        std::make_unique<Ros2ControlPluginTransmissionInstanceModel>(
          core, combo.interface_id, /*is_forward=*/true));
      transmission_analysis.add_transmission(
        fwd_model_id,
        actuator_sids,
        joint_sids,
        transmission.name + "/" + combo.interface_id.name + "[fwd]");
    }

    if (combo.inverse) {
      const auto inv_model_id = transmission_analysis.add_model(
        std::make_unique<Ros2ControlPluginTransmissionInstanceModel>(
          core, combo.interface_id, /*is_forward=*/false));
      transmission_analysis.add_transmission(
        inv_model_id,
        std::move(joint_sids),
        std::move(actuator_sids),
        transmission.name + "/" + combo.interface_id.name + "[inv]");
    }
  }
}

enum class MimicVisitState {
  Unvisited,
  Visiting,
  Done
};

TransmissionAnalysis::AffineTransmission normalize_affine_transmission(
  const StateInterfaceId target_joint_id,
  const std::unordered_map<StateInterfaceId, TransmissionAnalysis::AffineTransmission> & raw_affine_transmissions,
  std::unordered_map<StateInterfaceId, TransmissionAnalysis::AffineTransmission> & normalized_affine_transmissions,
  std::unordered_map<StateInterfaceId, MimicVisitState> & visit_states)
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
    normalized_source.source_joint_id,
    target_joint_id,
    normalized_source.multiplier * raw_it->second.multiplier,
    normalized_source.offset * raw_it->second.multiplier + raw_it->second.offset
  };

  visit_states[target_joint_id] = MimicVisitState::Done;
  normalized_affine_transmissions[target_joint_id] = normalized;
  return normalized;
}

} // namespace

void add_urdf_joints_to_analysis(TransmissionAnalysis & transmission_analysis, const urdf::Model & urdf_model) {
  for (const auto & [name, joint] : urdf_model.joints_) {
    if (joint->type == urdf::Joint::FIXED)
      continue;

    transmission_analysis.ensure_joint_id(name);
  }
}

void add_ros2_control_transmissions_to_analysis_dangerous(
  TransmissionAnalysis & transmission_analysis,
  const std::string & urdf_string,
  std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader)
{
  if (!plugin_loader) {
    plugin_loader = std::make_shared<Ros2ControlTransmissionPluginLoader>();
  }

  // TODO: There should be a separate registry for interface types outside of the AffineProjectionRules, as not all
  //       interfaces may be projected.
  // Source the per-import probe set from whatever interfaces the analysis currently has
  // registered AffineProjectionRules for. By default this is {position, velocity, acceleration},
  // populated by `TransmissionAnalysis`'s constructor; if a caller has registered an additional
  // rule (e.g. effort) before this import runs, the importer will probe and register the
  // corresponding ros2_control combos automatically. Snapshotted into a vector so the inner
  // probe loop iterates a stable contiguous sequence regardless of any (rare) interleaved
  // mutation later.
  std::vector<InterfaceId> probe_interfaces{};
  probe_interfaces.reserve(transmission_analysis.affine_projection_rules().size());
  for (const auto & [interface_id, _rule] : transmission_analysis.affine_projection_rules()) {
    probe_interfaces.push_back(interface_id);
  }

  for (auto & hardware_info : hardware_interface::parse_control_resources_from_urdf(urdf_string)) {
    for (auto & transmission : hardware_info.transmissions) {
      add_ros2_control_transmission_to_analysis(
        transmission_analysis, transmission, plugin_loader, probe_interfaces);
    }
  }
}

void add_ros2_control_transmissions_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const std::string & urdf_string,
  std::shared_ptr<const Ros2ControlTransmissionPluginLoader> plugin_loader,
  const rclcpp::Logger& logger)
{
  try {
    add_ros2_control_transmissions_to_analysis_dangerous(
      transmission_analysis,
      urdf_string,
      std::move(plugin_loader));
  } catch (const std::runtime_error & e) {
    RCLCPP_WARN(logger, "Error trying to add transmissions to TransmissionAnalysis: %s", e.what());
  }
}

void add_ros2_control_transmissions_to_analysis(
  TransmissionAnalysis & transmission_analysis,
  const std::string & urdf_string,
  const rclcpp::Logger & logger)
{
  add_ros2_control_transmissions_to_analysis(
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

  std::unordered_map<StateInterfaceId, TransmissionAnalysis::AffineTransmission> raw_affine_transmissions{};

  for (const auto & [joint_name, joint] : urdf_model.joints_) {
    if (!joint->mimic) {
      continue;
    }

    const auto target_joint_id = transmission_analysis.ensure_joint_id(joint_name);

    raw_affine_transmissions[target_joint_id] = TransmissionAnalysis::AffineTransmission{
      transmission_analysis.ensure_joint_id(joint->mimic->joint_name),
      target_joint_id,
      joint->mimic->multiplier,
      joint->mimic->offset
    };
  }

  std::unordered_map<StateInterfaceId, TransmissionAnalysis::AffineTransmission> normalized_affine_transmissions{};
  std::unordered_map<StateInterfaceId, MimicVisitState> visit_states{};

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
