//
// Created by Bailey Chessum on 14/10/2025.
//

#ifndef BANKSIA_KINEMATICS_PLUGIN_KINEMATICS_PLUGIN_BASE_HPP
#define BANKSIA_KINEMATICS_PLUGIN_KINEMATICS_PLUGIN_BASE_HPP

#include <string>
#include <vector>
#include <geometry_msgs/msg/pose.hpp>
#include <Eigen/Geometry>
#include <rclcpp/logger.hpp>
#include <rclcpp/node_interfaces/node_base_interface.hpp>
#include <rclcpp/node_interfaces/node_logging_interface.hpp>
#include "rclcpp/node_interfaces/node_interfaces.hpp"
#include <optional>
#include <urdf/model.h>
#include <kdl_parser/kdl_parser.hpp>
#include <kdl/jntarray.hpp>


namespace arm_kinematics {

/**
 * Parameters used to configure kinematics plugins
 */
struct KinematicsParams {

  /// The name of the link in the URDF to treat as the 'origin' for kinematics
  std::string base_link_name;

  /// The name of the link to use by default as the target in FK and IK calculations. Usually the end effector.
  std::string ee_link_name;
};

class KinematicsPluginBase {
public:
  using KinematicsNodeInterfaces =
    rclcpp::node_interfaces::NodeInterfaces<rclcpp::node_interfaces::NodeBaseInterface, rclcpp::node_interfaces::NodeLoggingInterface>;


  /**
   * Effectively replaces the constructor for the class, as we can only use a default constructor in plugins.
   *
   * Very expensive, and obviously not real-time safe.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  bool initialize(
    KinematicsNodeInterfaces node_interfaces,
    std::string & robot_description,
    const std::vector<std::string>& joint_names);

  virtual bool get_position_ik(
    const Eigen::Isometry3d & ik_pose,
    const std::vector<double> & ik_seed_state,
    std::vector<double> & solution_state) const = 0;

  virtual bool get_velocity_ik(
    const Eigen::Matrix<double, 6, 1> & ik_twist,
    const Eigen::Isometry3d & ik_seed_pose,
    const std::vector<double> & ik_seed_state,
    std::vector<double> & solution_velocities,
    double time_step = 0.01) const;

  virtual bool get_velocity_ik(
    const Eigen::Matrix<double, 6, 1> & ik_twist,
    const std::vector<double> & ik_seed_state,
    std::vector<double> & solution_velocities,
    double time_step = 0.01) const;

  /**
   * Do Forward Kinematics to find the position of a link with the given name.
   *
   * \param[in] joint_angles The current angle of every joint in get_joint_names(), in radians.
   * \param[in] link_name The name of the link to find the pose for.
   * \param[out] solution_pose The pose found through forward kinematics.
   *
   * \returns True if a solution could be found. False otherwise.
   */
  virtual bool get_position_fk(
    const std::vector<double> & joint_angles,
    const std::string & link_name,
    Eigen::Isometry3d & solution_pose) const;

  /**
    * Do Forward Kinematics to find the position of the main End Effector link.
    *
    * \param[in] joint_angles The current angle of every joint in get_joint_names(), in radians.
    * \param[out] solution_pose The pose found through forward kinematics.
    *
    * \returns True if a solution could be found. False otherwise.
    */
  virtual bool get_position_fk(
    const std::vector<double> & joint_angles,
    Eigen::Isometry3d & solution_pose) const;;

  // Accessors

  /// The URDF being used.
  [[nodiscard]] const std::string & get_robot_description() const;
  /// The name of every joint considered in IK and FK calculations.
  [[nodiscard]] const std::vector<std::string> & get_joint_names() const noexcept;
  /// Logger to use for logging
  [[nodiscard]] const rclcpp::Logger & get_logger() const noexcept;

  [[nodiscard]] const KinematicsParams & get_kinematics_params() const noexcept;

  [[nodiscard]] const KinematicsNodeInterfaces & get_node_interfaces() const;

protected:
  /**
   * Called when the kinematics plugin is created. Override this to add any set up logic for the kinematics plugin.
   *
   * \returns True if initialization was successful. False otherwise.
   */
  virtual bool on_initialize() = 0;

private:
  /// The URDF being used. You can get this from within a ros2_control controller
  std::string * robot_description_ = nullptr;

  KinematicsParams kinematics_params_;

  /// The name of every joint considered in IK and FK calculations.
  std::vector<std::string> joint_names_{};

  /// Allows us to access various things from the owning node if we need, like loggers, parameters, or in the future,
  /// maybe even topics.
  std::optional<KinematicsNodeInterfaces> node_interfaces_ = std::nullopt;

  /// Logger to use for logging
  rclcpp::Logger logger_ = rclcpp::get_logger("arm_kinematics");

  // KDL
  urdf::Model urdf_model_;
  KDL::Tree kdl_tree_;
  /// The default kdl chain to use, from the base link to the ee link
  KDL::Chain kdl_chain_;
  /// Pre-allocated KDL::JntArray to use for per-joint values
  KDL::JntArray preallocated_jnts{};
};

} // arm_kinematics

#endif //BANKSIA_KINEMATICS_PLUGIN_KINEMATICS_PLUGIN_BASE_HPP
