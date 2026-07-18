//
// Created by Bailey Chessum on 15/12/2025.
//

#ifndef ARM_KINEMATICS_ROBOT_MODEL_HPP
#define ARM_KINEMATICS_ROBOT_MODEL_HPP

#include <memory>
#include <mutex>
#include <string>

#include "arm_kinematics/utilities/lazy_once.hpp"
#include "arm_kinematics/visibility_control.h"

// Heavy types referenced only by accessor return types and member-storage forward references.
// Their full definitions live in the corresponding `.hpp`s; consumers that actually use these
// types include them directly. Keeping the `RobotModel` header light-weight keeps the public
// surface of the package from dragging in the entire joint_map subsystem on every consumer.
namespace urdf { class Model; }

namespace arm_kinematics {

class TransmissionAnalysis;
class AnalysisTree;
class Ros2ControlTransmissionPluginLoader;

/**
 * \brief Allows multiple plugins to share the same data structures derived from the robot's URDF.
 *
 * \warning This should be held by the parent node (your ROS2 control controller!) as a RobotModel::UniquePtr,
 * then should be passed to children by const ref. Do not reallocate this without reinitializing all of your kinematics!
 * \note Derived data is lazily evaluated using std::call_once, so should be thread safe.
 */
class ARM_KINEMATICS_PUBLIC RobotModel {
public:
  using UniquePtr = std::unique_ptr<RobotModel>;
  using SharedPtr = std::shared_ptr<RobotModel>;

  explicit RobotModel(std::string robot_description);

  // Out-of-line so the LazyOnce<T>-managed forward-declared types only need to be complete in
  // robot_model.cpp, not in every translation unit that includes this header.
  ~RobotModel();

  /// Accessor for robot_description.
  /// Variable not provided directly for consistency and the possibility of future change to internal type.
  [[nodiscard]] const std::string & get_robot_description() const;

  /// Lazily evaluated urdf model from parsing robot_description
  [[nodiscard]] const urdf::Model & get_urdf_model() const;

  /// Gets the lazily-built shared default transmission analysis derived from the robot
  /// description. The first call triggers URDF parsing and population (mimic + ros2_control
  /// transmissions); subsequent calls return the same instance. The analysis is logically
  /// frozen after lazy init — the `LazyOnce<T>` storage exists ONLY to support lazy population
  /// from a const method; callers must not assume ongoing mutation.
  [[nodiscard]] const TransmissionAnalysis & get_default_transmission_analysis() const;

  /// Gets the lazily-built shared ros2_control transmission plugin loader used by the default analysis import path.
  [[nodiscard]] std::shared_ptr<const Ros2ControlTransmissionPluginLoader>
  get_ros2_control_transmission_plugin_loader() const;

  /// Data structure modelling the tree of joints in the urdf, and how links in the urdf relate to those joints, lazily
  /// evaluated using get_urdf_model() as the constructor input
  [[nodiscard]] const AnalysisTree & get_analysis_tree() const;

private:
  /// TODO: I'm not huge on making a copy of the robot_description here, but it just seems more foolproof to copy.
  /// URDF describing the robot
  std::string robot_description_;

  /// Lazily evaluated URDF model.
  mutable LazyOnce<urdf::Model> urdf_model_{};

  /// Lazily evaluated shared default transmission analysis.
  mutable LazyOnce<TransmissionAnalysis> default_transmission_analysis_{};

  /// Lazily evaluated analysis tree.
  mutable LazyOnce<AnalysisTree> analysis_tree_{};

  /// Lazily evaluated shared ros2_control transmission plugin loader. The shared_ptr return
  /// type means LazyOnce<T> doesn't fit cleanly here (its semantics return `T &`), so this one
  /// stays hand-rolled.
  mutable std::shared_ptr<const Ros2ControlTransmissionPluginLoader> ros2_control_transmission_plugin_loader_{};
  mutable std::once_flag ros2_control_transmission_plugin_loader_flag_{};
};

}

#endif //ARM_KINEMATICS_ROBOT_MODEL_HPP
