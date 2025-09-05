#include <urdf_parser/urdf_parser.h>
#include "nova_arm_controller/self_collision_limiter.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"

namespace nova_arm_controller
{
// TODO: don't have same code here and in twistmapper
std::string SelfCollisionLimiter::construct_srdf_fallback_string(const urdf::ModelInterfaceSharedPtr &urdf_model,
                                                              std::string joint_group_name) {
  // TODO: Allow an SRDF to be provided
  std::ostringstream srdf_stream;
  srdf_stream << "<robot name=\"" << urdf_model->getName() << "\">\n";
  srdf_stream << "  <group name=\"" << joint_group_name << "\">\n";
  for (const auto& joint : this->joint_names_)
    srdf_stream << "    <joint name=\"" << joint << "\"/>\n";
  srdf_stream << "  </group>\n";
  srdf_stream << "</robot>\n";
  auto srdf_string = srdf_stream.str();
  return srdf_string;
}

bool SelfCollisionLimiter::init(
    const std::vector<std::string> & joint_names,
    const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr & param_itf,
    const rclcpp::node_interfaces::NodeLoggingInterface::SharedPtr & logging_itf,
    const std::string & robot_description) {

  number_of_joints_ = joint_names.size();
  joint_names_ = joint_names;
  //joint_limits_.resize(number_of_joints_);
  node_param_itf_ = param_itf;
  node_logging_itf_ = logging_itf;

  this->urdf_str_ = robot_description;
  return this->on_init();
}

bool SelfCollisionLimiter::on_configure(const trajectory_msgs::msg::JointTrajectoryPoint &) {
  if (this->urdf_str_.empty()) {
    RCLCPP_ERROR(this->node_logging_itf_->get_logger(), "urdf string is empty");
    return false;
  }

  auto urdf_model = urdf::parseURDF(this->urdf_str_);
  if (!urdf_model) {
    RCLCPP_ERROR(this->node_logging_itf_->get_logger(), "Failed to parse URDF");
    return false;
  }

  // Create an SRDF with a joint group for the joints
  std::shared_ptr<srdf::Model> srdf_model = std::make_shared<srdf::Model>();
  auto srdf_string = construct_srdf_fallback_string(urdf_model, "joints");
  srdf_model->initString(*urdf_model, srdf_string);

  // Finally create the robot model
  this->robot_model_ = std::make_shared<moveit::core::RobotModel>(urdf_model, srdf_model);

  // I think configure can be called more than once, don't leak memory.
  if (this->planning_scene_ != NULL) {
    delete this->planning_scene_;
  }
  this->planning_scene_ = new planning_scene::PlanningScene(this->robot_model_);
  generate_allowed_collision_matrix();

  // removing these lines may improve performance, but would mean we don't get
  // a warning saying what collided
  this->collision_request_.contacts = true; // calculate contacting pairs
  this->collision_request_.max_contacts = 100; // calculate more than one

  return true;
}

SelfCollisionLimiter::~SelfCollisionLimiter() {
  delete this->planning_scene_;
}

bool SelfCollisionLimiter::on_enforce(
    const trajectory_msgs::msg::JointTrajectoryPoint & current_joint_states,
    trajectory_msgs::msg::JointTrajectoryPoint & desired_joint_states,
    const rclcpp::Duration & dt) {

  const auto dt_seconds = dt.seconds();
  // negative or null is not allowed
  if (dt_seconds <= 0.0)
  {
    return false;
  }

  // check for required inputs combination
  bool has_desired_position = (desired_joint_states.positions.size() == number_of_joints_);
  bool has_desired_velocity = (desired_joint_states.velocities.size() == number_of_joints_);
  bool has_current_position = (current_joint_states.positions.size() == number_of_joints_);

  if (!(has_current_position && (has_desired_position || has_desired_velocity))) {
    return false;
  }

  //TODO: this state doesn't know about wheel/pivot/diffbar movement
  moveit::core::RobotState robot_state = this->planning_scene_->getCurrentStateNonConst();
  for (unsigned int i = 0; i < this->number_of_joints_; i++)
  {
    double target_pos;
    if (has_desired_position) {
      target_pos = desired_joint_states.positions.at(i);
    } else { // velocity
      // TODO: consider current velocity and expected accel
      // XXX: If real physical arm gets stuck in a collider, add a *1.1 or smth to the end of this so it overestimates velocity
      target_pos = current_joint_states.positions.at(i) + desired_joint_states.velocities.at(i)*dt_seconds;
    }
    robot_state.setVariablePosition(this->joint_names_.at(i), target_pos);
  }

  this->planning_scene_->setCurrentState(robot_state);
  this->collision_result_.clear();

  this->planning_scene_->checkSelfCollision(this->collision_request_, this->collision_result_);

  if (this->collision_result_.collision) {
      this->collision_result_.print();
      if (has_desired_position) {
        for (unsigned int i = 0; i < this->number_of_joints_; i++) {
          desired_joint_states.positions[i] = current_joint_states.positions[i];
        }
      }
      if (has_desired_velocity) {
        desired_joint_states.velocities = std::vector<double>(this->number_of_joints_,0);
      }
  }

  return this->collision_result_.collision;
}

void SelfCollisionLimiter::generate_allowed_collision_matrix() {
  auto acm = planning_scene_->getAllowedCollisionMatrix();

  // Get the current state
  moveit::core::RobotState& state = planning_scene_->getCurrentStateNonConst();
  state.setToDefaultValues();
  state.update();

  // Set up request/result
  collision_detection::CollisionRequest req;
  collision_detection::CollisionResult res;
  req.contacts = true;      // We get contact info to generate the allowed collision matrix from
  req.max_contacts = 1024;  // This was chosen arbitrarily as some large number

  // Perform self-collision check
  planning_scene_->checkSelfCollision(req, res, state);

  // Add all colliding pairs to the ACM
  for (const auto &contact_pair : res.contacts)
  {
    const auto &link1 = contact_pair.first.first;
    const auto &link2 = contact_pair.first.second;
    acm.setEntry(link1, link2, true);  // Mark this pair as allowed to collide
  }

  planning_scene_->setAllowedCollisionMatrix(acm);
}
} //namespace nova_arm_controller
