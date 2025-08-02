#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "diff_drive_controller/diff_drive_controller.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/LinearMath/Quaternion.h"

namespace
{
  constexpr auto DEFAULT_INPUT_TOPIC_TWIST = "/cmd_vel";
  constexpr auto DEFAULT_INPUT_TOPIC = "/drive_input";
  constexpr auto DEFAULT_COMMAND_OUT_TOPIC = "~/cmd_vel_out";
  constexpr auto DEFAULT_ODOMETRY_TOPIC = "~/odom";
  constexpr auto DEFAULT_TRANSFORM_TOPIC = "/tf";
} // namespace

namespace diff_drive_controller
{
  using namespace std::chrono_literals;
  using controller_interface::interface_configuration_type;
  using controller_interface::InterfaceConfiguration;
  using hardware_interface::HW_IF_POSITION;
  using hardware_interface::HW_IF_VELOCITY;
  using lifecycle_msgs::msg::State;

  DiffDriveController::DiffDriveController() : controller_interface::ControllerInterface() {}

  const char *DiffDriveController::drive_feedback_type() const
  {
    return params_.drive_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
  }

  const char *DiffDriveController::pivot_feedback_type() const
  {
    return params_.pivot_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
  }

  controller_interface::CallbackReturn DiffDriveController::on_init()
  {
    try
    {
      // Create the parameter listener and get the parameters
      param_listener_ = std::make_shared<ParamListener>(get_node());
      params_ = param_listener_->get_params();
    }
    catch (const std::exception &e)
    {
      fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
      return controller_interface::CallbackReturn::ERROR;
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  InterfaceConfiguration DiffDriveController::command_interface_configuration() const
  {
    std::vector<std::string> conf_names;
    for (const auto &joint_name : params_.left_drive_names)
    {
      conf_names.push_back(joint_name + "/" + HW_IF_VELOCITY);
    }
    for (const auto &joint_name : params_.right_drive_names)
    {
      conf_names.push_back(joint_name + "/" + HW_IF_VELOCITY);
    }
    for (const auto &joint_name : params_.left_pivot_names)
    {
      conf_names.push_back(joint_name + "/" + HW_IF_POSITION);
    }
    for (const auto &joint_name : params_.right_pivot_names)
    {
      conf_names.push_back(joint_name + "/" + HW_IF_POSITION);
    }
    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  InterfaceConfiguration DiffDriveController::state_interface_configuration() const
  {
    std::vector<std::string> conf_names;
    for (const auto &joint_name : params_.left_drive_names)
    {
      conf_names.push_back(joint_name + "/" + drive_feedback_type());
    }
    for (const auto &joint_name : params_.right_drive_names)
    {
      conf_names.push_back(joint_name + "/" + drive_feedback_type());
    }
    for (const auto &joint_name : params_.left_pivot_names)
    {
      conf_names.push_back(joint_name + "/" + pivot_feedback_type());
    }
    for (const auto &joint_name : params_.right_pivot_names)
    {
      conf_names.push_back(joint_name + "/" + pivot_feedback_type());
    }
    return {interface_configuration_type::INDIVIDUAL, conf_names};
  }

  controller_interface::return_type DiffDriveController::update(
      const rclcpp::Time &time, const rclcpp::Duration &period)
  {
    auto logger = get_node()->get_logger();
    if (get_lifecycle_state().id() == State::PRIMARY_STATE_INACTIVE)
    {
      if (!is_halted)
      {
        halt();
        is_halted = true;
      }
      return controller_interface::return_type::OK;
    }

    std::shared_ptr<geometry_msgs::msg::TwistStamped> last_twist_command_msg;
    std::shared_ptr<nova_interfaces::msg::DriveInputStamped> last_command_msg;

    double tmp1 = 0.0;
    double tmp2 = 0.0;
    double &linear_command = tmp1;
    double &angular_command = tmp2;

    max_d_vel = params_.linear.x.max_acceleration * period.seconds();

    // update parameters if they have changed
    if (param_listener_->is_old(params_))
    {
      params_ = param_listener_->get_params();
      // RCLCPP_INFO(logger, "Parameters were updated");
    }

    if (params_.enable_twist_cmd)
    {
      RCLCPP_INFO_ONCE(logger, "***Twist control***");
      received_twist_msg_ptr_.get(last_twist_command_msg);

      if (last_twist_command_msg == nullptr)
      {
        RCLCPP_WARN(logger, "Velocity message received was a nullptr.");
        return controller_interface::return_type::ERROR;
      }

      const auto age_of_last_command = time - last_twist_command_msg->header.stamp;
      // Brake if cmd_vel has timeout, override the stored command
      if (age_of_last_command > cmd_vel_timeout_)
      {
        last_twist_command_msg->twist.linear.x = 0.0;
        last_twist_command_msg->twist.angular.z = 0.0;
      }

      // command may be limited further by SpeedLimit,
      // without affecting the stored twist command
      geometry_msgs::msg::TwistStamped command = *last_twist_command_msg;
      linear_command = command.twist.linear.x;
      angular_command = command.twist.angular.z;

      auto &last_command = previous_twist_commands_.back().twist;
      auto &second_to_last_command = previous_twist_commands_.front().twist;
      limiter_linear_.limit(
          linear_command, last_command.linear.x, second_to_last_command.linear.x, period.seconds());
      limiter_angular_.limit(
          angular_command, last_command.angular.z, second_to_last_command.angular.z, period.seconds());

      previous_twist_commands_.pop();
      previous_twist_commands_.emplace(command);

      // Publish limited velocity
      if (publish_limited_twist_ && realtime_limited_twist_publisher_->trylock())
      {
        auto &limited_velocity_command = realtime_limited_twist_publisher_->msg_;
        limited_velocity_command.header.stamp = time;
        limited_velocity_command.twist = command.twist;
        realtime_limited_twist_publisher_->unlockAndPublish();
      }
    }
    else
    {
      RCLCPP_INFO_ONCE(logger, "***DriveInput control***");

      received_drive_input_msg_ptr_.get(last_command_msg);

      if (last_command_msg == nullptr)
      {
        RCLCPP_WARN(logger, "DriveInputStamped message received was a nullptr.");
        return controller_interface::return_type::ERROR;
      }

      nova_interfaces::msg::DriveInputStamped command = *last_command_msg;
      linear_command = command.drive_input.speed;
      angular_command = command.drive_input.radius;

      const auto age_of_last_command = time - last_command_msg->header.stamp;

      // Brake if cmd_vel has timeout, override the stored command
      if (age_of_last_command > cmd_vel_timeout_)
      {
        last_command_msg->drive_input.speed = 0.0;
        last_command_msg->drive_input.radius = 0.0;
      }

      auto &last_command = previous_commands_.back().drive_input;
      auto &second_to_last_command = previous_commands_.front().drive_input;
      limiter_linear_.limit(
          linear_command, last_command.speed, second_to_last_command.speed, period.seconds());
      limiter_angular_.limit(
          angular_command, last_command.radius, second_to_last_command.radius, period.seconds());

      previous_commands_.pop();
      previous_commands_.emplace(command);

      target_direction = command.drive_input.direction;
    }

    previous_update_timestamp_ = time;

    // Apply (possibly new) multipliers:
    const double wheel_separation = params_.wheel_separation_multiplier * params_.steering_track;
    const double left_wheel_radius = params_.left_wheel_radius_multiplier * params_.wheel_radius;
    const double right_wheel_radius = params_.right_wheel_radius_multiplier * params_.wheel_radius;

    if (params_.open_loop)
    {
      odometry_.updateOpenLoop(linear_command, angular_command, time);
    }
    else
    {
      double left_feedback_mean = 0.0;
      double right_feedback_mean = 0.0;

      for (size_t index = 0; index < static_cast<size_t>(params_.wheels_per_side); ++index)
      {
        const double left_feedback = registered_left_drive_handles_.at(index).state.get().get_value();
        const double right_feedback = registered_right_drive_handles_.at(index).state.get().get_value();

        if (std::isnan(left_feedback) && std::isnan(right_feedback))
        {
          RCLCPP_ERROR(
              logger, "Either the left or right wheel %s is invalid for index [%zu]", drive_feedback_type(),
              index);
          return controller_interface::return_type::ERROR;
        }

        left_feedback_mean += left_feedback;
        right_feedback_mean += right_feedback;
      }
      left_feedback_mean /= params_.wheels_per_side;
      right_feedback_mean /= params_.wheels_per_side;

      if (params_.drive_position_feedback)
      {
        odometry_.update(left_feedback_mean, right_feedback_mean, time);
      }
      else
      {
        odometry_.updateFromVelocity(
            left_feedback_mean * left_wheel_radius * period.seconds(),
            right_feedback_mean * right_wheel_radius * period.seconds(), time
            );
      }
    }

    tf2::Quaternion orientation;
    orientation.setRPY(0.0, 0.0, odometry_.getHeading());

    bool should_publish = false;
    try
    {
      if (previous_publish_timestamp_ + publish_period_ < time)
      {
        previous_publish_timestamp_ += publish_period_;
        should_publish = true;
      }
    }
    catch (const std::runtime_error &)
    {
      // Handle exceptions when the time source changes and initialize publish timestamp
      previous_publish_timestamp_ = time;
      should_publish = true;
    }

    if (should_publish)
    {
      if (realtime_odometry_publisher_->trylock())
      {
        auto &odometry_message = realtime_odometry_publisher_->msg_;
        odometry_message.header.stamp = time;
        odometry_message.pose.pose.position.x = odometry_.getX();
        odometry_message.pose.pose.position.y = odometry_.getY();
        odometry_message.pose.pose.orientation.x = orientation.x();
        odometry_message.pose.pose.orientation.y = orientation.y();
        odometry_message.pose.pose.orientation.z = orientation.z();
        odometry_message.pose.pose.orientation.w = orientation.w();
        odometry_message.twist.twist.linear.x = odometry_.getLinear();
        odometry_message.twist.twist.angular.z = odometry_.getAngular();
        realtime_odometry_publisher_->unlockAndPublish();
      }

      if (params_.enable_odom_tf && realtime_odometry_transform_publisher_->trylock())
      {
        auto &transform = realtime_odometry_transform_publisher_->msg_.transforms.front();
        transform.header.stamp = time;
        transform.transform.translation.x = odometry_.getX();
        transform.transform.translation.y = odometry_.getY();
        transform.transform.rotation.x = orientation.x();
        transform.transform.rotation.y = orientation.y();
        transform.transform.rotation.z = orientation.z();
        transform.transform.rotation.w = orientation.w();
        realtime_odometry_transform_publisher_->unlockAndPublish();
      }
    }

    // set_best_effort_velocity
    float d_vel = linear_command - best_effort_velocity;
    if (abs(d_vel) > max_d_vel)
    {
      best_effort_velocity += max_d_vel * (d_vel > 0 ? 1 : -1);
    }
    else
    {
      best_effort_velocity = linear_command;
    };

    angle_offset = atan(params_.steering_track / params_.wheel_base);

    float radius = angular_command * target_direction;
    
    if (radius == INFINITY || radius == -INFINITY || target_direction == 0)
    {
      for (size_t index = 0; index < static_cast<size_t>(params_.wheels_per_side); ++index)
      {
        registered_left_drive_handles_[index].command.get().set_value(best_effort_velocity/params_.wheel_radius);
        registered_right_drive_handles_[index].command.get().set_value(best_effort_velocity/params_.wheel_radius);

        registered_left_pivot_handles_[index].command.get().set_value(angle_offset * (index == 0 ? 1 : -1));
        registered_right_pivot_handles_[index].command.get().set_value(angle_offset * (index == 0 ? -1 : 1));
      }
    }
    else
    {
      // Calculate distances from the wheel_base centre to each wheel, and the maximum distance
      float left_wheel_distances[params_.wheels_per_side];
      float right_wheel_distances[params_.wheels_per_side];
      float wheel_x = params_.steering_track / 2;
      float wheel_y;

      float max_dist = 0;
      for (size_t index = 0; index < static_cast<size_t>(params_.wheels_per_side); ++index)
      {
        // position of wheel
        wheel_y = params_.wheel_base / 2 * (index == 0 ? 1 : -1);

        auto wheel_dist = [radius](float x, float y) -> float
        {
          return sqrt(pow(radius - x, 2) + pow(y, 2));
        };

        left_wheel_distances[index] = wheel_dist(-wheel_x, wheel_y);
        right_wheel_distances[index] = wheel_dist(wheel_x, wheel_y);

        max_dist = std::max({max_dist, left_wheel_distances[index], right_wheel_distances[index]});
      }
      // Handle sharp turning when radius is very small
      for (size_t index = 0; index < static_cast<size_t>(params_.wheels_per_side); ++index)
      {
       // Handle sharp turning when radius is very small
        bool sharp_turn = abs(radius) < (params_.steering_track) / 2;
        float left_speed = best_effort_velocity * left_wheel_distances[index] / max_dist;
        float right_speed = best_effort_velocity * right_wheel_distances[index] / max_dist;

        if (sharp_turn)
        {
          if (target_direction < 0) // Left turn
          {
            left_speed = -left_speed;
          }
          else if (target_direction > 0) // Right turn
          {
            right_speed = -right_speed;
          }
        }

        registered_left_drive_handles_[index].command.get().set_value(left_speed / params_.wheel_radius);
        registered_right_drive_handles_[index].command.get().set_value(right_speed / params_.wheel_radius);

        registered_left_pivot_handles_[index].command.get().set_value(angle_offset * (index == 0 ? 1 : -1));
        registered_right_pivot_handles_[index].command.get().set_value(angle_offset * (index == 0 ? -1 : 1));

        RCLCPP_INFO(logger, "Left wheel speed: %f, Right wheel speed: %f", left_speed / params_.wheel_radius, right_speed / params_.wheel_radius);
      }
    }

    return controller_interface::return_type::OK;
  }

  controller_interface::CallbackReturn DiffDriveController::on_configure(
      const rclcpp_lifecycle::State &)
  {
    auto logger = get_node()->get_logger();

    // update parameters if they have changed
    if (param_listener_->is_old(params_))
    {
      params_ = param_listener_->get_params();
      RCLCPP_INFO(logger, "Parameters were updated");
    }

    if (params_.left_drive_names.size() != params_.right_drive_names.size())
    {
      RCLCPP_ERROR(
          logger, "The number of left wheels [%zu] and the number of right wheels [%zu] are different",
          params_.left_drive_names.size(), params_.right_drive_names.size());
      return controller_interface::CallbackReturn::ERROR;
    }

    if (params_.left_drive_names.empty())
    {
      RCLCPP_ERROR(logger, "Wheel names parameters are empty!");
      return controller_interface::CallbackReturn::ERROR;
    }

    const double wheel_separation = params_.wheel_separation_multiplier * params_.steering_track;
    const double left_wheel_radius = params_.left_wheel_radius_multiplier * params_.wheel_radius;
    const double right_wheel_radius = params_.right_wheel_radius_multiplier * params_.wheel_radius;

    odometry_.setWheelParams(wheel_separation, left_wheel_radius, right_wheel_radius);
    odometry_.setVelocityRollingWindowSize(params_.velocity_rolling_window_size);

    cmd_vel_timeout_ = std::chrono::milliseconds{static_cast<int>(params_.cmd_vel_timeout * 1000.0)};
    publish_limited_twist_ = params_.publish_limited_velocity;
    // use_stamped_vel_ = params_.use_stamped_vel;

    limiter_linear_ = nova_controller_common::SpeedLimiter(
        params_.linear.x.has_velocity_limits, params_.linear.x.has_acceleration_limits,
        params_.linear.x.has_jerk_limits, params_.linear.x.min_velocity, params_.linear.x.max_velocity,
        params_.linear.x.min_acceleration, params_.linear.x.max_acceleration, params_.linear.x.min_jerk,
        params_.linear.x.max_jerk);

    limiter_angular_ = nova_controller_common::SpeedLimiter(
        params_.angular.z.has_velocity_limits, params_.angular.z.has_acceleration_limits,
        params_.angular.z.has_jerk_limits, params_.angular.z.min_velocity,
        params_.angular.z.max_velocity, params_.angular.z.min_acceleration,
        params_.angular.z.max_acceleration, params_.angular.z.min_jerk, params_.angular.z.max_jerk);

    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    // left and right sides are both equal at this point
    params_.wheels_per_side = params_.left_drive_names.size();

    if (publish_limited_twist_)
    {
      limited_twist_publisher_ =
          get_node()->create_publisher<geometry_msgs::msg::TwistStamped>(DEFAULT_COMMAND_OUT_TOPIC, rclcpp::SystemDefaultsQoS());
      realtime_limited_twist_publisher_ =
          std::make_shared<realtime_tools::RealtimePublisher<geometry_msgs::msg::TwistStamped>>(limited_twist_publisher_);
    }

    const geometry_msgs::msg::TwistStamped empty_twist;
    received_twist_msg_ptr_.set(std::make_shared<geometry_msgs::msg::TwistStamped>(empty_twist));

    const nova_interfaces::msg::DriveInputStamped empty_drive_input;
    received_drive_input_msg_ptr_.set(std::make_shared<nova_interfaces::msg::DriveInputStamped>(empty_drive_input));

    // Fill last two commands with default constructed commands
    previous_twist_commands_.emplace(empty_twist);
    previous_twist_commands_.emplace(empty_twist);

    // Fill last two commands with default constructed commands
    previous_commands_.emplace(empty_drive_input);
    previous_commands_.emplace(empty_drive_input);

    twist_subscriber_ = get_node()->create_subscription<geometry_msgs::msg::TwistStamped>(
    DEFAULT_INPUT_TOPIC_TWIST, rclcpp::SystemDefaultsQoS(),
    [this](const std::shared_ptr<geometry_msgs::msg::TwistStamped> msg) -> void
    {
        if (!subscriber_is_active_)
        {
        // RCLCPP_WARN(
        //     get_node()->get_logger(), "Can't accept new commands. subscriber is inactive");
        return;
        }
        if ((msg->header.stamp.sec == 0) && (msg->header.stamp.nanosec == 0))
        {
        RCLCPP_WARN_ONCE(
            get_node()->get_logger(),
            "Received TwistStamped with zero timestamp, setting it to current "
            "time, this message will only be shown once");
        msg->header.stamp = get_node()->get_clock()->now();
        }
        received_twist_msg_ptr_.set(std::move(msg));
    });


    drive_input_subscriber_ = get_node()->create_subscription<nova_interfaces::msg::DriveInputStamped>(
    DEFAULT_INPUT_TOPIC, rclcpp::SystemDefaultsQoS(),
    [this](const std::shared_ptr<nova_interfaces::msg::DriveInputStamped> msg) -> void
    {
        if (!subscriber_is_active_)
        {
        // RCLCPP_WARN(
        //     get_node()->get_logger(), "Can't accept new commands. subscriber is inactive");
        // return;
        }
        if ((msg->header.stamp.sec == 0) && (msg->header.stamp.nanosec == 0))
        {
        RCLCPP_WARN_ONCE(
            get_node()->get_logger(),
            "Received DriveInputStamped with zero timestamp, setting it to current "
            "time, this message will only be shown once");
        msg->header.stamp = get_node()->get_clock()->now();
        }
        received_drive_input_msg_ptr_.set(std::move(msg));
    });

    // initialize odometry publisher and messasge
    odometry_publisher_ = get_node()->create_publisher<nav_msgs::msg::Odometry>(
        DEFAULT_ODOMETRY_TOPIC, rclcpp::SystemDefaultsQoS());
    realtime_odometry_publisher_ =
        std::make_shared<realtime_tools::RealtimePublisher<nav_msgs::msg::Odometry>>(
            odometry_publisher_);

    // Append the tf prefix if there is one
    std::string tf_prefix = "";
    if (params_.tf_frame_prefix_enable)
    {
      if (params_.tf_frame_prefix != "")
      {
        tf_prefix = params_.tf_frame_prefix;
      }
      else
      {
        tf_prefix = std::string(get_node()->get_namespace());
      }

      if (tf_prefix == "/")
      {
        tf_prefix = "";
      }
      else
      {
        tf_prefix = tf_prefix + "/";
      }
    }

    const auto odom_frame_id = tf_prefix + params_.odom_frame_id;
    const auto base_frame_id = tf_prefix + params_.base_frame_id;

    auto &odometry_message = realtime_odometry_publisher_->msg_;
    odometry_message.header.frame_id = odom_frame_id;
    odometry_message.child_frame_id = base_frame_id;

    // limit the publication on the topics /odom and /tf
    publish_rate_ = params_.publish_rate;
    publish_period_ = rclcpp::Duration::from_seconds(1.0 / publish_rate_);

    // initialize odom values zeros
    odometry_message.twist =
        geometry_msgs::msg::TwistWithCovariance(rosidl_runtime_cpp::MessageInitialization::ALL);

    constexpr size_t NUM_DIMENSIONS = 6;
    for (size_t index = 0; index < 6; ++index)
    {
      // 0, 7, 14, 21, 28, 35
      const size_t diagonal_index = NUM_DIMENSIONS * index + index;
      odometry_message.pose.covariance[diagonal_index] = params_.pose_covariance_diagonal[index];
      odometry_message.twist.covariance[diagonal_index] = params_.twist_covariance_diagonal[index];
    }

    // initialize transform publisher and message
    odometry_transform_publisher_ = get_node()->create_publisher<tf2_msgs::msg::TFMessage>(
        DEFAULT_TRANSFORM_TOPIC, rclcpp::SystemDefaultsQoS());
    realtime_odometry_transform_publisher_ =
        std::make_shared<realtime_tools::RealtimePublisher<tf2_msgs::msg::TFMessage>>(
            odometry_transform_publisher_);

    // keeping track of odom and base_link transforms only
    auto &odometry_transform_message = realtime_odometry_transform_publisher_->msg_;
    odometry_transform_message.transforms.resize(1);
    odometry_transform_message.transforms.front().header.frame_id = odom_frame_id;
    odometry_transform_message.transforms.front().child_frame_id = base_frame_id;

    previous_update_timestamp_ = get_node()->get_clock()->now();
    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn DiffDriveController::on_activate(
      const rclcpp_lifecycle::State &)
  {
    const auto left_drives_result =
        configure_drive_pivots(params_.left_drive_names, registered_left_drive_handles_, drive_feedback_type());
    const auto right_drives_result =
        configure_drive_pivots(params_.right_drive_names, registered_right_drive_handles_, drive_feedback_type());
    const auto left_pivots_result =
        configure_drive_pivots(params_.left_pivot_names, registered_left_pivot_handles_, pivot_feedback_type());
    const auto right_pivots_result =
        configure_drive_pivots(params_.right_pivot_names, registered_right_pivot_handles_, pivot_feedback_type());

    if (
        left_drives_result == controller_interface::CallbackReturn::ERROR ||
        right_drives_result == controller_interface::CallbackReturn::ERROR ||
        left_pivots_result == controller_interface::CallbackReturn::ERROR ||
        right_pivots_result == controller_interface::CallbackReturn::ERROR)
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    if (registered_left_drive_handles_.empty() || registered_right_drive_handles_.empty())
    {
      RCLCPP_ERROR(
          get_node()->get_logger(),
          "Either left drive interfaces, right drive interfaces are non existent");
      return controller_interface::CallbackReturn::ERROR;
    }

    if (registered_left_pivot_handles_.empty() || registered_right_pivot_handles_.empty())
    {
      RCLCPP_ERROR(
          get_node()->get_logger(),
          "Either left pivot interfaces, right pivot interfaces are non existent");
      return controller_interface::CallbackReturn::ERROR;
    }

    is_halted = false;
    subscriber_is_active_ = true;

    RCLCPP_DEBUG(get_node()->get_logger(), "Subscriber and publisher are now active.");
    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn DiffDriveController::on_deactivate(
      const rclcpp_lifecycle::State &)
  {
    subscriber_is_active_ = false;
    if (!is_halted)
    {
      halt();
      is_halted = true;
    }
    registered_left_drive_handles_.clear();
    registered_right_drive_handles_.clear();
    registered_left_pivot_handles_.clear();
    registered_right_pivot_handles_.clear();
    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn DiffDriveController::on_cleanup(
      const rclcpp_lifecycle::State &)
  {
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    received_twist_msg_ptr_.set(std::make_shared<geometry_msgs::msg::TwistStamped>());
    received_drive_input_msg_ptr_.set(std::make_shared<nova_interfaces::msg::DriveInputStamped>());

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn DiffDriveController::on_error(const rclcpp_lifecycle::State &)
  {
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }
    return controller_interface::CallbackReturn::SUCCESS;
  }

  bool DiffDriveController::reset()
  {
    odometry_.resetOdometry();

    // release the old queue
    std::queue<geometry_msgs::msg::TwistStamped> empty_twist;
    std::swap(previous_twist_commands_, empty_twist);

    std::queue<nova_interfaces::msg::DriveInputStamped> empty;
    std::swap(previous_commands_, empty);

    registered_left_drive_handles_.clear();
    registered_right_drive_handles_.clear();
    registered_left_pivot_handles_.clear();
    registered_right_pivot_handles_.clear();

    subscriber_is_active_ = false;
    twist_subscriber_.reset();
    drive_input_subscriber_.reset();

    received_twist_msg_ptr_.set(nullptr);
    received_drive_input_msg_ptr_.set(nullptr);

    is_halted = false;
    return true;
  }

  controller_interface::CallbackReturn DiffDriveController::on_shutdown(
      const rclcpp_lifecycle::State &)
  {
    return controller_interface::CallbackReturn::SUCCESS;
  }

  void DiffDriveController::halt()
  {
    const auto halt_wheels = [](auto &wheel_handles)
    {
      for (const auto &wheel_handle : wheel_handles)
      {
        wheel_handle.command.get().set_value(0.0);
      }
    };

    halt_wheels(registered_left_drive_handles_);
    halt_wheels(registered_right_drive_handles_);
    halt_wheels(registered_left_pivot_handles_);
    halt_wheels(registered_right_pivot_handles_);
  }

  controller_interface::CallbackReturn DiffDriveController::configure_drive_pivots(
      const std::vector<std::string> &wheel_names,
      std::vector<WheelHandle> &registered_handles, const char *feedback_type)
  {
    auto logger = get_node()->get_logger();

    if (wheel_names.empty())
    {
      RCLCPP_ERROR(logger, "No wheel names specified");
      return controller_interface::CallbackReturn::ERROR;
    }

    // register handles
    registered_handles.reserve(wheel_names.size());
    for (const auto &wheel_name : wheel_names)
    {
      const auto interface_name = feedback_type;
      const auto state_handle = std::find_if(
          state_interfaces_.cbegin(), state_interfaces_.cend(),
          [&wheel_name, &interface_name](const auto &interface)
          {
            return interface.get_prefix_name() == wheel_name &&
                   interface.get_interface_name() == interface_name;
          });

      if (state_handle == state_interfaces_.cend())
      {
        RCLCPP_ERROR(logger, "Unable to obtain joint state handle for %s", wheel_name.c_str());
        return controller_interface::CallbackReturn::ERROR;
      }

      const auto command_handle = std::find_if(
          command_interfaces_.begin(), command_interfaces_.end(),
          [&wheel_name, &interface_name](const auto &interface)
          {
            return interface.get_prefix_name() == wheel_name &&
                   interface.get_interface_name() == interface_name;
          });

      if (command_handle == command_interfaces_.end())
      {
        RCLCPP_ERROR(logger, "Unable to obtain joint command handle for %s", wheel_name.c_str());
        return controller_interface::CallbackReturn::ERROR;
      }

      registered_handles.emplace_back(
          WheelHandle{std::ref(*state_handle), std::ref(*command_handle)});
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }
} // namespace diff_drive_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
    diff_drive_controller::DiffDriveController, controller_interface::ControllerInterface)
