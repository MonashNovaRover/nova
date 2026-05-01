#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <Eigen/Geometry>
#include <rclcpp/rclcpp.hpp>

#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/forward/frame_definitions.hpp"
#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"
#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"
#include "arm_kinematics/plugin_loader.hpp"
#include "arm_kinematics/utilities/aliases.hpp"

namespace arm_kinematics_benchmark {

inline constexpr const char * kBenchmarkDataWorkspaceRelativePath = "data/taipan_twistmapper_benchmark.urdf";
inline constexpr const char * kBenchmarkDataInstallRelativePath =
  "share/arm_kinematics_benchmark/benchmark/taipan_twistmapper_benchmark.urdf";
inline constexpr const char * kBenchmarkDataEnvVar = "ARM_KINEMATICS_BENCHMARK_DATA_DIR";
inline constexpr const char * kBaseLinkName = "arm_kinematics_origin";
inline constexpr const char * kEndEffectorLinkName = "endeffector_kinematics";
inline constexpr const char * kFallbackFrameId = "endeffector_kinematics";
inline constexpr double kSelfIntersectionMaxStepSize = 0.05;
inline constexpr double kVelocityIkTimeStep = 0.01;

extern const std::vector<std::string> kJointNames;
extern const std::vector<std::string> kIgnoredLinks;

struct RosInitGuard {
  RosInitGuard();
  ~RosInitGuard();
};

const RosInitGuard & ros_guard();

std::string benchmark_data_path();
std::string load_taipan_urdf();

std::shared_ptr<rclcpp::Node> make_benchmark_node(const std::string & name, bool enable_ik = false);

arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr make_single_frame_tree(
  arm_kinematics::ForwardKinematicsPlugin & fk,
  const std::string & frame_name);

struct TwistmapperCollisionRuntime {
  std::shared_ptr<rclcpp::Node> node;
  arm_kinematics::PluginLoader loader;
  arm_kinematics::ForwardKinematicsPlugin::SharedPtr fk;
  arm_kinematics::CollisionManager collision_manager;
  std::vector<double> current_joint_state_values;
  std::vector<double> predicted_joint_positions;
  std::vector<double> joint_values_scratch;
  std::vector<std::pair<std::size_t, std::size_t>> colliding_pairs_scratch;

  explicit TwistmapperCollisionRuntime(const std::string & node_name);
};

struct TwistmapperUpdateRuntime {
  std::shared_ptr<rclcpp::Node> node;
  arm_kinematics::PluginLoader loader;
  arm_kinematics::ForwardKinematicsPlugin::SharedPtr fk;
  arm_kinematics::InverseKinematicsPlugin::SharedPtr ik;
  arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr ee_tree;
  arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr twist_frame_tree;
  arm_kinematics::Isometry3dVector fk_pose_buffer{1};
  std::vector<double> current_joint_state{0.0, -0.35, 0.4, 0.1, -0.25, 0.2};
  std::vector<double> solution_positions = std::vector<double>(kJointNames.size(), 0.0);
  std::vector<double> solution_velocities = std::vector<double>(kJointNames.size(), 0.0);
  Eigen::Isometry3d seed_ee_pose{Eigen::Isometry3d::Identity()};
  arm_kinematics::Twistd twist_with_rotation;
  arm_kinematics::Twistd twist_linear_only;

  explicit TwistmapperUpdateRuntime(const std::string & node_name);
};

}  // namespace arm_kinematics_benchmark
