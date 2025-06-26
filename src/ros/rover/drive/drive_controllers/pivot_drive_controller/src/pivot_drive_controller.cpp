#include <cstdio>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include <tuple>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/LinearMath/Quaternion.h"

#include "pivot_drive_controller/pivot_drive_controller.hpp"

namespace
{
    constexpr auto DEFAULT_INPUT_TOPIC = "/drive_input";
    constexpr auto DEFAULT_INPUT_TOPIC_TWIST = "/cmd_vel";
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

    const char *PivotDriveController::drive_feedback_type() const
    {
      return params_.drive_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
    }

    const char *PivotDriveController::pivot_feedback_type() const
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
        catch (const std::exception &e)
        {
            fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
            return controller_interface::CallbackReturn::ERROR;
        }



        zero_radius_ = sqrt(params_.wheel_base*params_.wheel_base/4 + params_.steering_track*params_.steering_track/4);
        RCLCPP_INFO_STREAM(get_node()->get_logger(), "zero_radius_: " << zero_radius_);
        angle_offset_ = atan(params_.steering_track / params_.wheel_base);
        RCLCPP_INFO_STREAM(get_node()->get_logger(), "angle_offset_: " << angle_offset_);

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
        // #TODO: Our radius based commands take left as negative and right as positive. This
        //        this is opposite to the correct angular commands in the frame of the rover
        //        We should make right the negative direction to match
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

        max_d_theta = params_.max_theta * period.seconds();

        // RCLCPP_INFO(logger, "period: %f", period.seconds());

        std::shared_ptr<geometry_msgs::msg::TwistStamped> last_twist_command_msg;
        std::shared_ptr<nova_interfaces::msg::DriveInputStamped> last_command_msg;

        nova_interfaces::msg::DriveInputStamped command;

        double target_radius, target_direction, target_speed;
        
        double left_angle, right_angle;

        if (param_listener_->is_old(params_))
        {
            params_ = param_listener_->get_params();
            // RCLCPP_INFO(logger, "Parameters were updated");
        }
        
        // RCLCPP_INFO(logger, "enable_twist_cmd: %d", params_.enable_twist_cmd);

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
            double linear_command = twist_command.twist.linear.x;
            double angular_command = twist_command.twist.angular.z;

            auto & last_command = previous_twist_commands_.back().twist;
            auto & second_to_last_command = previous_twist_commands_.front().twist;

            limiter_linear_.limit(
                linear_command, last_command.linear.x, second_to_last_command.linear.x, period.seconds()
            );

            //TODO: angular limiter

            //previous_twist_commands only ever contains x2 values
            previous_twist_commands_.pop();
            previous_twist_commands_.emplace(twist_command);

            target_radius = angular_command == 0 ? INFINITY : abs(linear_command / angular_command);
//            RCLCPP_DEBUG(logger, "Target_radius: %f", target_radius);
//
//            RCLCPP_DEBUG(logger, "Target_speed: %f", target_speed);

            //target_radius = angular_command == 0 ? 0 : linear_command / abs(angular_command);
            target_direction = angular_command > 0 ? -1 : 1;

            target_speed = target_radius == 0 ? abs(angular_command) * zero_radius_ : linear_command;

        } else {
            received_drive_input_msg_ptr_.get(last_command_msg);

            if (last_command_msg == nullptr)
            {
                RCLCPP_WARN(logger, "DriveInputStamped message received was a nullptr.");
                return controller_interface::return_type::ERROR;
            }

            command = *last_command_msg;
            target_speed = command.drive_input.speed;
            target_radius = command.drive_input.radius;
            target_direction = command.drive_input.direction;

            const auto age_of_last_command = time - last_command_msg->header.stamp;

            //RCLCPP_INFO(logger, "time: %f, age_of_last_command: %f", time, age_of_last_command);

            // Brake if drive_input_cmd has timeout, override the stored command
            if (age_of_last_command > cmd_vel_timeout_)
            {
                last_command_msg->drive_input.speed = 0.0;
                last_command_msg->drive_input.radius = INFINITY;
            } 

            auto & last_command = previous_commands_.back().drive_input;

            auto & second_to_last_command = previous_commands_.front().drive_input;

            limiter_linear_.limit(
                target_speed, last_command.speed, second_to_last_command.speed, period.seconds()
            );
            //TODO: position limiter? (for pivots)

            //previous_commands only ever contains x2 values
            previous_commands_.pop();
            previous_commands_.emplace(command);
        }

        previous_update_timestamp_ = time;

        if (target_direction == 0) target_direction = 1;

//       RCLCPP_INFO(get_node()->get_logger(), "Target radius of %f and direction of %f", target_radius, target_direction);

        float radius;
        int direction;

        std::tie(radius, direction) = get_best_effort_radius_direction(target_radius,target_direction);
        //RCLCPP_INFO(get_node()->get_logger(), "best_effort radius of %f and direction of %f", radius, direction);

        left_angle = get_pivot_angle_from_radius(radius, true, direction);
        right_angle = get_pivot_angle_from_radius(radius, false, direction);

//        RCLCPP_INFO(get_node()->get_logger(), "left_angle command: %f", left_angle);
//        RCLCPP_INFO(get_node()->get_logger(), "right_angle command: %f", right_angle);

        registered_left_pivot_handles_.at(0).command.get().set_value(left_angle);
        registered_left_pivot_handles_.at(1).command.get().set_value(-left_angle);
        registered_right_pivot_handles_.at(0).command.get().set_value(-right_angle);
        registered_right_pivot_handles_.at(1).command.get().set_value(right_angle);
        

        // Update Odometry
        if (params_.open_loop)
        {
            // #TODO: Fix open loop odom
            float angular_command = (target_speed / radius) * direction * -1;
            //RCLCPP_INFO(logger, "time: %f", time);
            odometry_.updateOpenLoop(target_speed, angular_command, time);
        }
        else
        {
            // #TODO Make odometry fault tolerant (or at least recognise faults)
            const double front_right_wheel_value = registered_right_drive_handles_.at(0).state.get().get_value()*params_.wheel_radius;
            const double rear_right_wheel_value = registered_right_drive_handles_.at(1).state.get().get_value()*params_.wheel_radius;
            const double front_left_wheel_value = registered_left_drive_handles_.at(0).state.get().get_value()*params_.wheel_radius;
            const double rear_left_wheel_value = registered_left_drive_handles_.at(1).state.get().get_value()*params_.wheel_radius;

            const double front_right_steer_position = registered_right_pivot_handles_.at(0).state.get().get_value();
            const double rear_right_steer_position = registered_right_pivot_handles_.at(1).state.get().get_value();
            const double front_left_steer_position = registered_left_pivot_handles_.at(0).state.get().get_value();
            const double rear_left_steer_position = registered_left_pivot_handles_.at(1).state.get().get_value();


            if (
                !std::isnan(front_right_wheel_value) && !std::isnan(front_left_wheel_value) &&
                !std::isnan(rear_right_wheel_value) && !std::isnan(rear_left_wheel_value) &&
                !std::isnan(front_right_steer_position) && !std::isnan(front_left_steer_position) &&
                !std::isnan(rear_right_steer_position) && !std::isnan(rear_left_steer_position))
            {
                if (params_.pivot_position_feedback)
                {
                     RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "frw: " << front_right_wheel_value <<
                                                                  " flw: " << front_left_wheel_value <<
                                                                  " rrw: " << rear_right_wheel_value <<
                                                                  " rlw: " << rear_left_wheel_value);
                     RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "frp: " << front_right_steer_position <<
                                                                          " flp: " << front_left_steer_position <<
                                                                          " rrp: " << rear_right_steer_position <<
                                                                          " rlp: " << rear_left_steer_position);

                    // #TODO: move to the update function in odometry class
                    bool flp_left = front_left_steer_position < angle_offset_;
                    bool frp_left = front_right_steer_position > angle_offset_;
                    bool rlp_left = rear_left_steer_position < angle_offset_;
                    bool rrp_left = rear_right_steer_position > angle_offset_;

                    double frp_radius = get_radius_from_angle(-front_right_steer_position, frp_left) * (flp_left ? -1 : 1);
                     RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "frp_radius: " << frp_radius);
                    double flp_radius = get_radius_from_angle(front_left_steer_position, flp_left) * (flp_left ? -1 : 1);
                     RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "flp_radius: " << flp_radius);
                    double rrp_radius = get_radius_from_angle(rear_right_steer_position, rrp_left) * (flp_left ? -1 : 1);
                     RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "rrp_radius: " << rrp_radius);
                    double rlp_radius = get_radius_from_angle(-rear_left_steer_position, rlp_left) * (flp_left ? -1 : 1);
                     RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "rlp_radius: " << rlp_radius);
                    double mean_radius = (frp_radius + flp_radius + rrp_radius + rlp_radius) / 4;




                    if ((fabs(front_left_steer_position - angle_offset_) < 0.001) |
                        (fabs(front_right_steer_position + angle_offset_) < 0.001) |
                        (fabs(rear_left_steer_position + angle_offset_) < 0.001) |
                        (fabs(rear_right_steer_position - angle_offset_) < 0.001))
                    {
                        mean_radius = INFINITY;
                    }

                     RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "mean_radius: " << mean_radius);

                    if (fabs(mean_radius) < 0.1) mean_radius = 0;

                    double left_ratio =1;
                    double right_ratio = 1;
                    double max_ratio;

                    if (mean_radius != 0 && mean_radius != INFINITY) {
                        left_ratio = sqrt(pow(params_.wheel_base / 2, 2.0) +
                                          pow(mean_radius + (params_.steering_track / 2), 2.0))/abs(mean_radius);
                        right_ratio = sqrt(pow(params_.wheel_base / 2, 2.0) +
                                           pow(mean_radius - (params_.steering_track / 2), 2.0))/abs(mean_radius);
                    }

                    // #TODO: More robust system for invalid radius detection
                    if (!isnan(left_ratio) && !isnan(right_ratio)){
                         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "left_ratio: " << left_ratio);
                         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "right_ratio: " << right_ratio);
                         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "max_ratio: " << max_ratio);

                        max_ratio = std::max(abs(left_ratio), abs(right_ratio));

                        double mean_speed = (front_right_wheel_value/(right_ratio) +
                                             rear_right_wheel_value/(right_ratio) +
                                             front_left_wheel_value/(left_ratio) +
                                             rear_left_wheel_value/(left_ratio)) / 4;

                        // #TODO: Paramatize this
                        if (fabs(mean_speed) < 0.01) mean_speed = 0;

                         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "mean_speed: " << mean_speed);

                        double angular;
                        
                        // TODO: Fix this direction to be consistent with the frame.
                        if (mean_speed == 0 || mean_radius == INFINITY) {
                            angular = 0;
                        } else if ( mean_radius == 0) {
                            angular = (mean_speed/zero_radius_) * (flp_left ? 1 : -1);
                        } else {
                            angular = mean_speed/-mean_radius;
                        }

                        double linear = mean_radius == 0 ? 0 : mean_speed;

                         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "linear: " << linear);
                         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "angular: " << angular);

                        odometry_.update_odometry(linear, angular , period.seconds());
                    }

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

        float left_ratio =1, right_ratio = 1;

        if (radius != 0 && radius != INFINITY) {
            left_ratio = sqrt(pow(params_.wheel_base / 2, 2.0) +
                    pow(radius*direction + (params_.steering_track / 2), 2.0))/radius;
            right_ratio = sqrt(pow(params_.wheel_base / 2, 2.0) +
                    pow(radius*direction - (params_.wheel_base / 2), 2.0))/radius;
        }

        double left_velocity = target_speed*left_ratio;
        double right_velocity = target_speed*right_ratio;
        double max_velocity = std::max(abs(left_velocity), abs(right_velocity));

        RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "Left Velocity: " << left_velocity);
        RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "Right Velocity: " <<right_velocity);

        if (params_.has_velocity_limits && (abs(left_velocity) > params_.max_velocity || abs(right_velocity) > params_.max_velocity))
        {
            left_velocity = left_velocity/max_velocity * params_.max_velocity;
            right_velocity = right_velocity/max_velocity * params_.max_velocity;
            RCLCPP_WARN_STREAM(get_node()->get_logger(), "Velocity limit exceeded, scaling velocity down to "
            << left_velocity/left_ratio << " and " << right_velocity/right_ratio);
        }

        for (size_t index = 0; index < static_cast<size_t>(params_.wheels_per_side); ++index)
        {
            registered_left_drive_handles_.at(index).command.get().set_value(left_velocity/params_.wheel_radius);
            registered_right_drive_handles_.at(index).command.get().set_value(right_velocity/params_.wheel_radius);
        }

        prev_dir = target_direction;
        prev_radius = target_radius;

        return controller_interface::return_type::OK;
    }

    double PivotDriveController::get_pivot_angle_from_radius(float radius, bool left, int dir)
    {
        //RCLCPP_INFO(get_node()->get_logger(), "**get_pivot_angle_from_radius**");
        //RCLCPP_INFO(get_node()->get_logger(), "Left: %d, radius: %f, dir: %d\n-----", left, radius, dir);

        double angle;
        if(left){
            angle = (radius == INFINITY ? 0 : -(atan((2*radius*dir + params_.steering_track)/params_.wheel_base) - dir * M_PI_2)) + angle_offset_;
        } else {
            angle = (radius == INFINITY ? 0 : atan((2*radius*dir - params_.steering_track)/params_.wheel_base) - dir * M_PI_2) + angle_offset_;
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
            int dir = angle > angle_offset_ ? 1 : -1;
            radius = (angle == angle_offset_ ? INFINITY : (tan(-angle + angle_offset_ + M_PI_2) * params_.wheel_base - params_.steering_track)/(2*dir));
        } else {
            int dir = angle > angle_offset_ ? -1 : 1;
            radius = (angle == angle_offset_ ? INFINITY : (tan(angle - angle_offset_ + M_PI_2) * params_.wheel_base + params_.steering_track)/(2*dir));
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

           //float current_pivot_angle = i == 0 ? registered_left_pivot_handles_[0].state.get().get_value() : registered_right_pivot_handles_[1].state.get().get_value();
           float current_pivot_angle = get_pivot_angle_from_radius(prev_radius, i == 0, prev_dir);

           
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
                best_dir = best_effort_angle > angle_offset_ ? 1 : -1;
           } else if (i == 1)
           {
                best_dir = best_effort_angle > angle_offset_ ? -1 : 1;
           }

           float best_radius = abs(get_radius_from_angle(best_effort_angle, i == 0));
           //RCLCPP_INFO(get_node()->get_logger(), "best_radius for %d: %f", i, best_radius);

           double left_angle = get_pivot_angle_from_radius(best_radius, true, best_dir);
//           RCLCPP_INFO_STREAM(get_node()->get_logger(), "best_left_angle: " << left_angle);
//           RCLCPP_INFO_STREAM(get_node()->get_logger(), "curr_left_angle: " << registered_left_pivot_handles_[0].state.get().get_value());
           double right_angle = get_pivot_angle_from_radius(best_radius, false, best_dir);
//            RCLCPP_INFO_STREAM(get_node()->get_logger(), "best_right_angle: " << left_angle);
//            RCLCPP_INFO_STREAM(get_node()->get_logger(), "curr_right_angle: " << registered_right_pivot_handles_[0].state.get().get_value());
          /*
           bool valid = (abs(left_angle - registered_left_pivot_handles_[0].state.get().get_value()) <= max_d_theta*1.01) &&
               (abs(right_angle - registered_right_pivot_handles_[1].state.get().get_value()) <= max_d_theta * 1.01);
               */

           bool valid = (abs(left_angle -  get_pivot_angle_from_radius(prev_radius, true, prev_dir)) <= max_d_theta * 1.01) &&
             (abs(right_angle - get_pivot_angle_from_radius(prev_radius, false, prev_dir)) <= max_d_theta * 1.01);


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
        
        limiter_linear_ = nova_controller_common::SpeedLimiter(
            params_.has_velocity_limits, params_.has_acceleration_limits,
            params_.has_jerk_limits, params_.min_velocity, params_.max_velocity,
            params_.min_acceleration, params_.max_acceleration, params_.min_jerk,
            params_.max_jerk);
        limiter_angular_ = nova_controller_common::SpeedLimiter(
            params_.has_velocity_limits, params_.has_acceleration_limits,
            params_.has_jerk_limits, params_.min_velocity, params_.max_velocity,
            params_.min_acceleration, params_.max_acceleration, params_.min_jerk,
            params_.max_jerk);

        if (!reset())
        {
            return controller_interface::CallbackReturn::ERROR;
        }

        nova_interfaces::msg::DriveInputStamped empty_drive_input;
        empty_drive_input.drive_input.radius = INFINITY;
        empty_drive_input.drive_input.direction = 0;
        empty_drive_input.drive_input.speed = 0;

        const geometry_msgs::msg::TwistStamped empty_twist;

        // Fill last two commands with default constructed commands
        received_twist_msg_ptr_.set(std::make_shared<geometry_msgs::msg::TwistStamped>(empty_twist));
        received_drive_input_msg_ptr_.set(std::make_shared<nova_interfaces::msg::DriveInputStamped>(empty_drive_input));

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
            RCLCPP_WARN_ONCE(
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


        drive_input_subscriber_ = get_node()->create_subscription<nova_interfaces::msg::DriveInputStamped>(
        DEFAULT_INPUT_TOPIC, rclcpp::SystemDefaultsQoS(),
        [this](const std::shared_ptr<nova_interfaces::msg::DriveInputStamped> msg) -> void
        {
            if (!subscriber_is_active_)
            {
            RCLCPP_WARN_ONCE(
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

        received_drive_input_msg_ptr_.set(std::make_shared<nova_interfaces::msg::DriveInputStamped>());
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
        std::queue<nova_interfaces::msg::DriveInputStamped> empty;
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
