//
// Created by Bailey Chessum on 15/12/2025.
//

#ifndef ARM_KINEMATICS_ROBOT_MODEL_HPP
#define ARM_KINEMATICS_ROBOT_MODEL_HPP

#include <memory>
#include <mutex>
#include <urdf/model.h>

#include "arm_kinematics/visibility_control.h"
#include "arm_kinematics/forward/utilities/analysis_tree.hpp"
#include "arm_kinematics/joint_map/joint_map_builder.hpp"

namespace arm_kinematics {

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

  /// Accessor for robot_description.
  /// Variable not provided directly for consistency and the possibility of future change to internal type.
  [[nodiscard]] const std::string & get_robot_description() const;

  /// Lazily evaluated urdf model from parsing robot_description
  [[nodiscard]] const urdf::Model & get_urdf_model() const;

  /// Gets the standard joint map builder constructed from robot_description and get_urdf_model()
  [[nodiscard]] const JointMapBuilder & get_joint_map_builder() const;

  /// Data structure modelling the tree of joints in the urdf, and how links in the urdf relate to those joints, lazily
  /// evaluated using get_urdf_model() as the constructor input
  [[nodiscard]] const AnalysisTree & get_analysis_tree() const;

private:
  /// TODO: I'm not huge on making a copy of the robot_description here, but it just seems more foolproof to copy.
  /// URDF describing the robot
  std::string robot_description_;

  /// Lazily evaluated URDF model retrieved from get_urdf_model()
  mutable std::unique_ptr<urdf::Model> urdf_model_ = nullptr;
  mutable std::once_flag urdf_model_flag_{};

  /// Lazily evaluated joint map builder
  mutable std::unique_ptr<JointMapBuilder> joint_map_builder_ = nullptr;
  mutable std::once_flag joint_map_builder_flag_{};

  /// Lazily evaluated analysis tree
  mutable std::unique_ptr<AnalysisTree> analysis_tree_ = nullptr;
  mutable std::once_flag analysis_tree_flag_{};
};

}

#endif //ARM_KINEMATICS_ROBOT_MODEL_HPP