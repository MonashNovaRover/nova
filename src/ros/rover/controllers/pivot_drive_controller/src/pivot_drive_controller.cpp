#include <cstdio>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include <tuple>

#include "pivot_drive_controller/pivot_drive_controller.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/LinearMath/Quaternion.h"

namespace
{
    constexpr auto DEFAULT_INPUT_TOPIC = "/drive_input";
    constexpr auto DEFAULT_INPUT_TOPIC_STAMPED = "/drive_input_stamped";
    constexpr auto DEFAULT_INPUT_TOPIC_TWIST = "/cmd_vel";
    constexpr auto DEFAULT_INPUT_TOPIC_TWIST_STAMPED = "/cmd_vel_stamped";
    constexpr auto DEFAULT_OUTPUT_TOPIC = "~/cmd_vel_out";
    constexpr auto DEFAULT_ODOMETRY_TOPIC = "~/odom";
    constexpr auto DEFAULT_TRANSFORM_TOPIC = "~/tf";
}

namespace pivot_drive_controller
{
    using namespace std::chrono_literals;
    using controller_interface::interface_configuration_type;
    using controller_interface::InterfaceConfiguration;
    using hardware_interface::HW_IF_POSITION;
    using hardware_interface::HW_IF_VELOCITY;
    using lifecycle_msgs::msg::State;

    PivotDriveController::PivotDriveController() : controller_interface::ControllerInterface() {}

    const char * PivotDriveController::drive_feedback_type() const
    {
      return params_.drive_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
    }

    const char * PivotDriveController::pivot_feedback_type() const
    {
      return params_.pivot_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
    }

    controller_interface::CallbackReturn PivotDriveController::on_init()
    {
        try
        {
            // Create the parameter listener and get the parameters
            param_listener_ = std::make_shared<ParamListener>(get_node());
            params_ = param_listener_->get_params();
        }
        catch (const std::exception & e)
        {
            fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
            return controller_interface::CallbackReturn::ERROR;
        }

        return controller_interface::CallbackReturn::SUCCESS;
    }

    InterfaceConfiguration PivotDriveController::command_interface_configuration() const
    {
        std::vector<std::string> conf_names;
        for (const auto & joint_name : params_.left_drive_names)
        {
            conf_names.push_back(joint_name + "/" + HW_IF_VELOCITY);
        }
        for (const auto & joint_name : params_.right_drive_names)
        {
            conf_names.push_back(joint_name + "/" + HW_IF_VELOCITY);
        }
        for (const auto & joint_name : params_.left_pivot_names)
        {
            conf_names.push_back(joint_name + "/" + HW_IF_POSITION);
        }
        for (const auto & joint_name : params_.right_pivot_names)
        {
            conf_names.push_back(joint_name + "/" + HW_IF_POSITION);
        }
        return {interface_configuration_type::INDIVIDUAL, conf_names};
    }

    InterfaceConfiguration PivotDriveController::state_interface_configuration() const
    {
        std::vector<std::string> conf_names;
        for (const auto & joint_name : params_.left_drive_names)
        {
            conf_names.push_back(joint_name + "/" + drive_feedback_type());
        }
        for (const auto & joint_name : params_.right_drive_names)
        {
            conf_names.push_back(joint_name + "/" + drive_feedback_type());
        }
        for (const auto & joint_name : params_.left_pivot_names)
        {
            conf_names.push_back(joint_name + "/" + pivot_feedback_type());
        }
        for (const auto & joint_name : params_.right_pivot_names)
        {
            conf_names.push_back(joint_name + "/" + pivot_feedback_type());
        }
        return {interface_configuration_type::INDIVIDUAL, conf_names};
    }

    controller_interface::return_type PivotDriveController::update(
        const rclcpp::Time & time, const rclcpp::Duration & period)
    {
        auto logger = get_node()->get_logger();
        
        if (get_state().id() == State::PRIMARY_STATE_INACTIVE)
        {
            if (!is_halted)
            {
                halt();
                is_halted = true;
            }

            return controller_interface::return_type::OK;
        }

        max_d_theta = params_.max_theta * period.seconds();

        //RCLCPP_INFO(logger, "period: %f", period.seconds());

        std::shared_ptr<geometry_msgs::msg::TwistStamped> last_twist_command_msg;
        std::shared_ptr<core::msg::DriveInputStamped> last_command_msg;

        double tmp1 = 0.0;
        double tmp2 = 0.0;
        double tmp3 = 0.0;
        double & linear_command = tmp1;
        double & angular_command = tmp2;
        double & speed = tmp3;

        core::msg::DriveInputStamped command;

        float target_radius, target_direction;
        angle_offset = atan(params_.steering_track / params_.wheel_base);

        double left_angle, right_angle;

        if (param_listener_->is_old(params_))
        {
            params_ = param_listener_->get_params();
            RCLCPP_INFO(logger, "Parameters were updated");
        }

        if (params_.enable_twist_cmd) {
            received_twist_msg_ptr_.get(last_twist_command_msg);

            if (last_twist_command_msg == nullptr)
            {
                RCLCPP_WARN(logger, "Twist message received was a nullptr.");
                return controller_interface::return_type::ERROR;
            }

            const auto age_of_last_command = time - last_twist_command_msg->header.stamp;
            if (age_of_last_command > cmd_vel_timeout_)
            {
                last_twist_command_msg->twist.linear.x = 0.0;
                last_twist_command_msg->twist.angular.z = 0.0;
            } 

            geometry_msgs::msg::TwistStamped twist_command = *last_twist_command_msg;
            linear_command = twist_command.twist.linear.x;
            angular_command = twist_command.twist.angular.z;

            auto & last_command = previous_twist_commands_.back().twist;
            auto & second_to_last_command = previous_twist_commands_.front().twist;
            limiter_linear_.limit(
                linear_command, last_command.linear.x, second_to_last_command.linear.x, period.seconds()
            );

            target_radius = angular_command == 0 ? INFINITY : abs(linear_command / angular_command);

            speed = linear_command + (linear_command >= 0 ? 1 : -1) * abs((params_.wheel_base / 2) * angular_command);
            //TODO: angular limiter

            //previous_twist_commands only ever contains x2 values
            previous_twist_commands_.pop();
            previous_twist_commands_.emplace(twist_command);

            //target_radius = angular_command == 0 ? 0 : linear_command / abs(angular_command);
            target_direction = angular_command * linear_command > 0 ? -1 : 1;

        } else {
            RCLCPP_INFO(logger, "DriveInput control.");
            received_drive_input_msg_ptr_.get(last_command_msg);

            if (last_command_msg == nullptr)
            {
                RCLCPP_WARN(logger, "DriveInputStamped message received was a nullptr.");
                return controller_interface::return_type::ERROR;
            }

            command = *last_command_msg;
            speed = command.drive_input.speed;

            const auto age_of_last_command = time - last_command_msg->header.stamp;

            //RCLCPP_INFO(logger, "time: %f, age_of_last_command: %f", time, age_of_last_command);

            // Brake if drive_input_cmd has timeout, override the stored command
            if (age_of_last_command > cmd_vel_timeout_)
            {
                last_command_msg->drive_input.speed = 0.0;
                last_command_msg->drive_input.radius = 0.0;
            } 

            auto & last_command = previous_commands_.back().drive_input;
            auto & second_to_last_command = previous_commands_.front().drive_input;

            limiter_linear_.limit(
                speed, last_command.speed, second_to_last_command.speed, period.seconds()
            );
            //TODO: position limiter? (for pivots)

            //previous_commands only ever contains x2 values
            previous_commands_.pop();
            previous_commands_.emplace(command);

            if(second_to_last_command.mode == core::msg::DriveInput::STRAFE && command.drive_input.mode == core::msg::DriveInput::PIVOT){
                RCLCPP_INFO(logger, "switching from strafe to pivot drive");
                target_radius = INFINITY;
                target_direction = 0;

                //initialise all pivot angles
                for (size_t index = 0; index < static_cast<size_t>(params_.wheels_per_side); ++index)
                {
                    registered_left_pivot_handles_.at(index).command.get().set_value(angle_offset);
                    registered_right_pivot_handles_.at(index).command.get().set_value(angle_offset);
                }
                
            } else {
                target_radius = command.drive_input.radius;
                target_direction = command.drive_input.direction;
            }
        }

        previous_update_timestamp_ = time;

        if (target_direction == 0) target_direction = 1;

        //RCLCPP_INFO(get_node()->get_logger(), "Target radius of %f and direction of %f", target_radius, target_direction);
        float radius;
        int direction;

        std::tie(radius, direction) = get_best_effort_radius_direction(target_radius,target_direction);

        left_angle = get_pivot_angle_from_radius(radius, true, direction);
        right_angle = get_pivot_angle_from_radius(radius, false, direction);

//        RCLCPP_INFO_STREAM(get_node()->get_logger(), "left_angle command: " << left_angle);
//        RCLCPP_INFO_STREAM(get_node()->get_logger(), "right_angle command: " << right_angle);

        registered_left_pivot_handles_.at(0).command.get().set_value(left_angle);
        registered_left_pivot_handles_.at(1).command.get().set_value(-left_angle);
        registered_right_pivot_handles_.at(0).command.get().set_value(-right_angle);
        registered_right_pivot_handles_.at(1).command.get().set_value(right_angle);

        // Update Odometry
        if (params_.open_loop)
        {
            float angular_command = (speed / radius) * direction * -1;
            float linear_command = speed - (speed >= 0 ? 1 : -1) * abs((params_.wheel_base / 2) * angular_command);
            //RCLCPP_INFO(logger, "time: %f", time);
            odometry_.updateOpenLoop(linear_command, angular_command, time);
        }
        else
        {
            const double front_right_wheel_value = registered_right_drive_handles_.at(0).state.get().get_value();
            const double rear_right_wheel_value = registered_right_drive_handles_.at(1).state.get().get_value();
            const double front_left_wheel_value = registered_left_drive_handles_.at(0).state.get().get_value();
            const double rear_left_wheel_value = registered_left_drive_handles_.at(1).state.get().get_value();

            const double front_right_steer_position = registered_right_pivot_handles_.at(0).state.get().get_value();
            const double rear_right_steer_position = registered_right_pivot_handles_.at(1).state.get().get_value();
            const double front_left_steer_position = registered_left_pivot_handles_.at(0).state.get().get_value();
            const double rear_left_steer_position = registered_left_pivot_handles_.at(1).state.get().get_value();

            //RCLCPP_INFO(logger, "wheel values: %f, %f, %f, %f", front_right_wheel_value, rear_right_wheel_value, front_left_wheel_value, rear_left_wheel_value);
            //RCLCPP_INFO(logger, "steer values: %f, %f, %f, %f", front_right_steer_position, rear_right_steer_position, front_left_steer_position, rear_left_steer_position);

            if (
                !std::isnan(front_right_wheel_value) && !std::isnan(front_left_wheel_value) &&
                !std::isnan(rear_right_wheel_value) && !std::isnan(rear_left_wheel_value) &&
                !std::isnan(front_right_steer_position) && !std::isnan(front_left_steer_position) &&
                !std::isnan(rear_right_steer_position) && !std::isnan(rear_left_steer_position))
            {
                if (params_.pivot_position_feedback)
                {
                    double front_steer_position = 0.0;
                    if (fabs(front_right_steer_position) > 0.001 || fabs(front_left_steer_position) > 0.001)
                    {
                        front_steer_position = atan(2 * tan(front_right_steer_position) * tan(front_left_steer_position) /
                                                    (tan(front_right_steer_position) + tan(front_left_steer_position)));
                    }
                    double rear_steer_position = 0.0;
                    if (fabs(rear_right_steer_position) > 0.001 || fabs(rear_left_steer_position) > 0.001)
                    {
                        rear_steer_position = atan(2 * tan(rear_right_steer_position) * tan(rear_left_steer_position) /
                                                   (tan(rear_right_steer_position) + tan(rear_left_steer_position)));
                    }

                    //RCLCPP_INFO(logger, "updating odometry with front_steer_position of %f and rear_steer_position of %f", front_steer_position, rear_steer_position);
                    // Estimate linear and angular velocity using joint information
                    odometry_.update(
                        front_left_wheel_value, front_right_wheel_value, rear_left_wheel_value, rear_right_wheel_value,
                        front_steer_position, rear_steer_position, period.seconds());
                } 
            }
        }

        tf2::Quaternion orientation;
        orientation.setRPY(0.0, 0.0, odometry_.getHeading());
        //RCLCPP_INFO(logger, "heading: %f", odometry_.getHeading());

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
//            RCLCPP_INFO(logger, "should_publish");
            if (realtime_odometry_publisher_->trylock())
            {
                auto & odometry_message = realtime_odometry_publisher_->msg_;
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
        
        float left_ratio =1;
        float right_ratio = 1;
        float max_ratio;

        if (radius != 0 && radius != INFINITY) {
            left_ratio = sqrt(pow(params_.wheel_base / 2, 2.0) +
                    pow(radius*direction + (params_.steering_track / 2), 2.0))/radius;
            right_ratio = sqrt(pow(params_.wheel_base / 2, 2.0) +
                    pow(radius*direction - (params_.wheel_base / 2), 2.0))/radius;
        }

        max_ratio = std::max(abs(left_ratio), abs(right_ratio));

        for (size_t index = 0; index < static_cast<size_t>(params_.wheels_per_side); ++index)
        {
            registered_left_drive_handles_.at(index).command.get().set_value(linear_command * left_ratio/max_ratio);
            registered_right_drive_handles_.at(index).command.get().set_value(linear_command * right_ratio/max_ratio);
        }
        return controller_interface::return_type::OK;
    }

    double PivotDriveController::get_pivot_angle_from_radius(float radius, bool left, int dir)
    {
        //RCLCPP_INFO(get_node()->get_logger(), "**get_pivot_angle_from_radius**");
        //RCLCPP_INFO(get_node()->get_logger(), "Left: %d, radius: %f, dir: %d\n-----", left, radius, dir);

        double angle;
        if(left){
            angle = (radius == INFINITY ? 0 : -(atan((2*radius*dir + params_.steering_track)/params_.wheel_base) - dir * M_PI_2)) + angle_offset;
        } else {
            angle = (radius == INFINITY ? 0 : atan((2*radius*dir - params_.steering_track)/params_.wheel_base) - dir * M_PI_2) + angle_offset;
        }
        return angle;
    }

    double PivotDriveController::get_radius_from_angle(double angle, bool left) 
    {
        double radius;

        //if the absolute value of the angle is greater than 90 degrees, radius is 0
        if (abs(angle) >= M_PI_2) return 0;

        // inverse of the math in calc_wheel_angle
        if(left){
            int dir = angle > angle_offset ? 1 : -1;
            radius = (angle == angle_offset ? INFINITY : (tan(-angle + angle_offset + M_PI_2) * params_.wheel_base - params_.steering_track)/(2*dir));
        } else {
            int dir = angle > angle_offset ? -1 : 1;
            radius = (angle == angle_offset ? INFINITY : (tan(angle - angle_offset + M_PI_2) * params_.wheel_base + params_.steering_track)/(2*dir));
        }

        return radius;
    }

    // Gets the turning radius of the rover
    std::tuple<float,int> PivotDriveController::get_best_effort_radius_direction(float target_radius, float target_direction) 
    {
        //RCLCPP_INFO(get_node()->get_logger(),"**get_best_effort_radius_direction**");

        std::tuple<float,int,bool> best_efforts_left_right[2]; //array of {radius, direction, valid} for front left and front right pivots
        int drive_dir, best_dir, pivot_with_best_radius = 0;

        //RCLCPP_INFO(get_node()->get_logger(), "target radius: %f, target direction: %f", target_radius, target_direction);

        //iterate through each front pivot
        for (int i = 0; i < 2; i++) 
        {
           //calc angle for turning radius 
           double target_angle = get_pivot_angle_from_radius(target_radius, i == 0, target_direction);
           //RCLCPP_INFO(get_node()->get_logger(), "target angle: %f", target_angle);

           float current_pivot_angle = i == 0 ? registered_left_pivot_handles_[0].state.get().get_value() : registered_right_pivot_handles_[1].state.get().get_value();
           
           //determine drive direction to reach target_angle
           if (current_pivot_angle < target_angle) {
               drive_dir = 1;
           } else if (current_pivot_angle > target_angle) {
               drive_dir = -1;
           } else {
                drive_dir = 0;
           }

           //calculate max. angle of pivot
//           RCLCPP_INFO_STREAM(get_node()->get_logger(), "max_d_theta in get_best_effort_radius_direction: " << max_d_theta);
           double best_effort_angle = current_pivot_angle + drive_dir * max_d_theta;

           if (abs(current_pivot_angle - target_angle) < max_d_theta)
           {
                best_effort_angle = target_angle;
           }

//            RCLCPP_INFO(get_node()->get_logger(), "best_effort_angle for %d: %f", i, best_effort_angle);
//           RCLCPP_INFO(get_node()->get_logger(), "current_pivot_angle for %d: %f", i, current_pivot_angle);
//              RCLCPP_INFO(get_node()->get_logger(), "target_angle for %d: %f", i, target_angle);
           //calculate direction
           if (current_pivot_angle == target_angle)
           {
                best_dir = 0;

           } else if (i == 0)
           {
                best_dir = best_effort_angle > angle_offset ? 1 : -1;
           } else if (i == 1)
           {
                best_dir = best_effort_angle > angle_offset ? -1 : 1;
           }

           float best_radius = abs(get_radius_from_angle(best_effort_angle, i == 0));
           //RCLCPP_INFO(get_node()->get_logger(), "best_radius for %d: %f", i, best_radius);

           double left_angle = get_pivot_angle_from_radius(best_radius, true, best_dir);
//           RCLCPP_INFO_STREAM(get_node()->get_logger(), "best_left_angle: " << left_angle);
//           RCLCPP_INFO_STREAM(get_node()->get_logger(), "curr_left_angle: " << registered_left_pivot_handles_[0].state.get().get_value());
           double right_angle = get_pivot_angle_from_radius(best_radius, false, best_dir);
//            RCLCPP_INFO_STREAM(get_node()->get_logger(), "best_right_angle: " << left_angle);
//            RCLCPP_INFO_STREAM(get_node()->get_logger(), "curr_right_angle: " << registered_right_pivot_handles_[0].state.get().get_value());
           bool valid = (abs(left_angle - registered_left_pivot_handles_[0].state.get().get_value()) <= max_d_theta*1.01) &&
               (abs(right_angle - registered_right_pivot_handles_[1].state.get().get_value()) <= max_d_theta * 1.01);

           best_efforts_left_right[i] = {best_radius, best_dir, valid};
        }

        
        //calculate which radius is 'best' to use
        pivot_with_best_radius = 0; //0 = left, 1 = right
        for (int i =0; i < 2; i++) {
            //RCLCPP_INFO(get_node()->get_logger(), "best_effort_left_right[%d], dir: %f", i, std::get<1>(best_efforts_left_right[i]));

            //if this configuration is invalid
            if (std::get<2>(best_efforts_left_right[i]) == 0) {
                return {std::get<0>(best_efforts_left_right[1-i]), std::get<1>(best_efforts_left_right[1-i])};
            }

            if (i == 1) {
                float radius_difference_1 = abs(std::get<0>(best_efforts_left_right[i])*std::get<1>(best_efforts_left_right[i]) - target_radius * target_direction);
                float radius_difference_0 = abs(std::get<0>(best_efforts_left_right[0])*std::get<1>(best_efforts_left_right[0]) - target_radius * target_direction);

                if (radius_difference_1 < radius_difference_0) {
                    pivot_with_best_radius = 1;
                }
            }
        }

        //RCLCPP_INFO(get_node()->get_logger(), "pivot_with_best_radius: %d, dir: %f", pivot_with_best_radius, std::get<1>(best_efforts_left_right[pivot_with_best_radius]));
        return std::make_tuple(std::get<0>(best_efforts_left_right[pivot_with_best_radius]), std::get<1>(best_efforts_left_right[pivot_with_best_radius]));
    }

    controller_interface::CallbackReturn PivotDriveController::on_configure(
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

        cmd_vel_timeout_ = std::chrono::milliseconds{static_cast<int>(params_.cmd_vel_timeout * 1000.0)};
        
        limiter_linear_ = SpeedLimiter(
            params_.has_velocity_limits, params_.has_acceleration_limits,
            params_.has_jerk_limits, params_.min_velocity, params_.max_velocity,
            params_.min_acceleration, params_.max_acceleration, params_.min_jerk,
            params_.max_jerk);

        if (!reset())
        {
            return controller_interface::CallbackReturn::ERROR;
        }

        const core::msg::DriveInputStamped empty_drive_input;
        const geometry_msgs::msg::TwistStamped empty_twist;

        // Fill last two commands with default constructed commands
        received_twist_msg_ptr_.set(std::make_shared<geometry_msgs::msg::TwistStamped>(empty_twist));
        received_drive_input_msg_ptr_.set(std::make_shared<core::msg::DriveInputStamped>(empty_drive_input));

        previous_twist_commands_.emplace(empty_twist);
        previous_twist_commands_.emplace(empty_twist);

        // Fill last two commands with default constructed commands
        previous_commands_.emplace(empty_drive_input);
        previous_commands_.emplace(empty_drive_input);

        twist_unstamped_subscriber_ = get_node()->create_subscription<geometry_msgs::msg::Twist>(
          DEFAULT_INPUT_TOPIC_TWIST, rclcpp::SystemDefaultsQoS(),
          [this](const std::shared_ptr<geometry_msgs::msg::Twist> msg) -> void
          {
            if (!params_.use_unstamped_msg) {
              return;
            }
            if (!subscriber_is_active_)
            {
              RCLCPP_WARN(
                get_node()->get_logger(), "Can't accept new commands. subscriber is inactive");
              return;
            }

            // Write fake header in the stored stamped command
            std::shared_ptr<geometry_msgs::msg::TwistStamped> twist_stamped;
            received_twist_msg_ptr_.get(twist_stamped);
            twist_stamped->twist = *msg;
            twist_stamped->header.stamp = get_node()->get_clock()->now();
          });


        drive_input_unstamped_subscriber_ = get_node()->create_subscription<core::msg::DriveInput>(
          DEFAULT_INPUT_TOPIC, rclcpp::SystemDefaultsQoS(),
          [this](const std::shared_ptr<core::msg::DriveInput> msg) -> void
          {
            if (!params_.use_unstamped_msg) {
              return;
            }
            if (!subscriber_is_active_)
            {
              RCLCPP_WARN(
                get_node()->get_logger(), "Can't accept new commands. subscriber is inactive");
              return;
            }

            // Write fake header in the stored stamped command
            std::shared_ptr<core::msg::DriveInputStamped> drive_input_stamped;
            received_drive_input_msg_ptr_.get(drive_input_stamped);
            drive_input_stamped->drive_input = *msg;
            drive_input_stamped->header.stamp = get_node()->get_clock()->now();
          });
        twist_subscriber_ = get_node()->create_subscription<geometry_msgs::msg::TwistStamped>(
          DEFAULT_INPUT_TOPIC_TWIST_STAMPED, rclcpp::SystemDefaultsQoS(),
          [this](const std::shared_ptr<geometry_msgs::msg::TwistStamped> msg) -> void
          {
            if (params_.use_unstamped_msg) {
              return;
            }
            if (!subscriber_is_active_)
            {
              RCLCPP_WARN(
                get_node()->get_logger(), "Can't accept new commands. subscriber is inactive");
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

        drive_input_subscriber_ = get_node()->create_subscription<core::msg::DriveInputStamped>(
          DEFAULT_INPUT_TOPIC_STAMPED, rclcpp::SystemDefaultsQoS(),
          [this](const std::shared_ptr<core::msg::DriveInputStamped> msg) -> void
          {
            if (params_.use_unstamped_msg) {
              return;
            }
            if (!subscriber_is_active_)
            {
              RCLCPP_WARN(
                get_node()->get_logger(), "Can't accept new commands. subscriber is inactive");
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

        auto & odometry_message = realtime_odometry_publisher_->msg_;
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
        auto & odometry_transform_message = realtime_odometry_transform_publisher_->msg_;
        odometry_transform_message.transforms.resize(1);
        odometry_transform_message.transforms.front().header.frame_id = odom_frame_id;
        odometry_transform_message.transforms.front().child_frame_id = base_frame_id;

        previous_update_timestamp_ = get_node()->get_clock()->now();
        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::CallbackReturn PivotDriveController::on_activate(
        const rclcpp_lifecycle::State &)
    {

        RCLCPP_INFO(get_node()->get_logger(),"on activate");
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
            right_pivots_result == controller_interface::CallbackReturn::ERROR )
        {
            RCLCPP_ERROR(get_node()->get_logger(), "Error configuring drives and pivots");
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

        RCLCPP_INFO(get_node()->get_logger(), "Subscriber and publisher are now active.");
        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::CallbackReturn PivotDriveController::on_deactivate(
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

    controller_interface::CallbackReturn PivotDriveController::on_cleanup(
        const rclcpp_lifecycle::State &)
    {
        if (!reset())
        {
            return controller_interface::CallbackReturn::ERROR;
        }

        received_drive_input_msg_ptr_.set(std::make_shared<core::msg::DriveInputStamped>());
        received_twist_msg_ptr_.set(std::make_shared<geometry_msgs::msg::TwistStamped>());

        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::CallbackReturn PivotDriveController::on_error(const rclcpp_lifecycle::State &)
    {
        if (!reset())
        {
            return controller_interface::CallbackReturn::ERROR;
        }
        return controller_interface::CallbackReturn::SUCCESS;
    }

    bool PivotDriveController::reset()
    {
        odometry_.resetOdometry();

        // release the old queue
        std::queue<core::msg::DriveInputStamped> empty;
        std::queue<geometry_msgs::msg::TwistStamped> empty_twist;
        std::swap(previous_commands_, empty);
        std::swap(previous_twist_commands_, empty_twist);

        registered_left_drive_handles_.clear();
        registered_right_drive_handles_.clear();
        registered_left_pivot_handles_.clear();
        registered_right_pivot_handles_.clear();

        subscriber_is_active_ = false;
        drive_input_subscriber_.reset();
        twist_subscriber_.reset();

        RCLCPP_DEBUG(get_node()->get_logger(), "reset");

        received_drive_input_msg_ptr_.set(nullptr);
        received_twist_msg_ptr_.set(nullptr);

        is_halted = false;
        return true;
    }

    controller_interface::CallbackReturn PivotDriveController::on_shutdown(
        const rclcpp_lifecycle::State &)
    {
        return controller_interface::CallbackReturn::SUCCESS;
    }

    void PivotDriveController::halt()
    {
        const auto halt_wheels = [](auto & wheel_handles)
        {
            for (const auto & wheel_handle : wheel_handles)
            {
            wheel_handle.command.get().set_value(0.0);
            }
        };

        halt_wheels(registered_left_drive_handles_);
        halt_wheels(registered_right_drive_handles_);
        halt_wheels(registered_left_pivot_handles_);
        halt_wheels(registered_right_pivot_handles_);
    }

    controller_interface::CallbackReturn PivotDriveController::configure_drive_pivots(
        const std::vector<std::string> & wheel_names,
        std::vector<WheelHandle> & registered_handles, const char * feedback_type)
    {
        auto logger = get_node()->get_logger();

        if (wheel_names.empty())
        {
            RCLCPP_ERROR(logger, "No wheel names specified");
            return controller_interface::CallbackReturn::ERROR;
        }

        // register handles
        registered_handles.reserve(wheel_names.size());
        for (const auto & wheel_name : wheel_names)
        {
            const auto interface_name = feedback_type;
            const auto state_handle = std::find_if(
                state_interfaces_.cbegin(), state_interfaces_.cend(), 
                [&wheel_name, &interface_name](const auto & interface)
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
                [&wheel_name, &interface_name](const auto & interface)
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
    } //namespace pivot_drive_controller
}

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
            pivot_drive_controller::PivotDriveController, controller_interface::ControllerInterface
        )
