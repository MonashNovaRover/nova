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
    constexpr auto DEFAULT_INPUT_TOPIC = "~/drive_input_cmd"; //no idea what to call this
    constexpr auto DEFAULT_OUTPUT_TOPIC = "~/drive_input_cmd_out";
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

    const char * PivotDriveController::feedback_type() const
    {
      return params_.position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
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
            conf_names.push_back(joint_name + "/" + feedback_type());
        }
        for (const auto & joint_name : params_.right_drive_names)
        {
            conf_names.push_back(joint_name + "/" + feedback_type());
        }
        for (const auto & joint_name : params_.left_pivot_names)
        {
            conf_names.push_back(joint_name + "/" + feedback_type());
        }
        for (const auto & joint_name : params_.right_pivot_names)
        {
            conf_names.push_back(joint_name + "/" + feedback_type());
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

        std::shared_ptr<core::msg::DriveInputStamped> last_command_msg;
        received_drive_input_msg_ptr_.get(last_command_msg);

       
        /*
        std::shared_ptr<geometry_msgs::msg::Twist> last_twist_command_msg;
        received_twist_msg_ptr_.get(last_twist_command_msg);


        if (last_twist_command_msg == nullptr)
        {
            RCLCPP_WARN(logger, "Twist message received was a nullptr.");
            return controller_interface::return_type::ERROR;
        }
        */
        
        
        if (last_command_msg == nullptr)
        {
            RCLCPP_WARN(logger, "DriveInputStamped message received was a nullptr.");
            return controller_interface::return_type::ERROR;
        }

        //const auto age_of_last_command = time - last_command_msg->header.stamp;
        const auto age_of_last_command = time - last_command_msg->header.stamp;
        // Brake if drive_input_cmd has timeout, override the stored command
        if (age_of_last_command > cmd_vel_timeout_)
        {
            last_command_msg->speed = 0.0;
            last_command_msg->radius = 0.0;
        } 

        // command may be limited further by SpeedLimit,
        // without affecting the stored DriveInputStamped command
        core::msg::DriveInputStamped command = *last_command_msg;
    
        float & linear_command = command.speed;


        previous_update_timestamp_ = time;

        // Apply (possibly new) multipliers:
        //const double wheel_separation = params_.wheel_separation_multiplier * params_.wheel_separation;
        //const double left_wheel_radius = params_.left_wheel_radius_multiplier * params_.wheel_radius;
        //const double right_wheel_radius = params_.right_wheel_radius_multiplier * params_.wheel_radius;

        // **********ODOMETRY UPDATE STUFF HERE *****************

        auto & last_command = previous_commands_.back();
        auto & second_to_last_command = previous_commands_.front();

        limiter_linear_.limit(
            linear_command, last_command.speed, second_to_last_command.speed, period.seconds());
       

        /* autonomous mode
        limiter_drive_.limit(
            command.angular, last_command.angular, second_to_last_command.angular, period.seconds());
            */

        //previous_commands only ever contains x2 values
        previous_commands_.pop();
        previous_commands_.emplace(command);

        //const core::msg::DriveInputStamped empty_drive_input;

        
        //    Publish limited velocity
        /*
        if (publish_limited_drive_pivot_ && realtime_limited_drive_pivot_publisher_->trylock())
        {
            auto & limited_drive_pivot_command = realtime_limited_drive_pivot_publisher_->msg_;
            limited_drive_pivot_command.header.stamp = time;
            limited_drive_pivot_command.speed = command.speed;
            limited_drive_pivot_command.radius = command.radius;
            realtime_limited_drive_pivot_publisher_->unlockAndPublish();
        }
        */
        
        float target_radius, target_direction;

        angle_offset = params_.steering_track / params_.wheel_base;
        if(second_to_last_command.mode == core::msg::DriveInputStamped::STRAFE && command.mode == core::msg::DriveInputStamped::PIVOT){
            RCLCPP_INFO(logger, "switching from strafe to pivot drive");
            target_radius = INFINITY;
            target_direction = 0;

            //initialise all pivot angles
            for (size_t index = 0; index < static_cast<size_t>(params_.wheels_per_side); ++index)
            {
                registered_left_pivot_handles_.at(index).command.get().set_value(angle_offset);
                registered_right_pivot_handles_.at(index).command.get().set_value(angle_offset);

                /*
                registered_left_pivot_handles_[index].command.get().set_value(angle_offset);
                registered_right_pivot_handles_[index].command.get().set_value(angle_offset);
                */
            }
            
        } else {
            target_radius = command.radius;
            target_direction = command.direction;
        }

        //don't need this if command.speed isn't a percentage
        //float target_velocity = params_.max_speed * command.speed; //command.speed is a value between 0--1 (or -1--1, not sure)
        auto [radius, direction] = get_best_effort_radius_direction(target_radius,target_direction);

        double left_angle = get_pivot_angle_from_radius(radius, true, direction);
        double right_angle = get_pivot_angle_from_radius(radius, false, direction);

        //set pivot angles
        for (size_t index = 0; index < static_cast<size_t>(params_.wheels_per_side); ++index)
        {
            registered_left_pivot_handles_.at(index).command.get().set_value(left_angle);
            registered_right_pivot_handles_.at(index).command.get().set_value(right_angle);

            /*
            registered_left_pivot_handles_[index].command.get().set_value(left_angle);
            registered_right_pivot_handles_[index].command.get().set_value(right_angle);
            */
        }

        //set drive velocities
        
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
            registered_left_drive_handles_.at(index).command.get().set_value(command.speed * left_ratio/max_ratio);
            registered_right_drive_handles_.at(index).command.get().set_value(command.speed * right_ratio/max_ratio);

            /*
            registered_left_drive_handles_[index].command.get().set_value(command.speed * left_ratio/max_ratio);
            registered_right_drive_handles_[index].command.get().set_value(command.speed * right_ratio/max_ratio);
            */
        }


        return controller_interface::return_type::OK;
    }

    double PivotDriveController::get_pivot_angle_from_radius(float radius, bool left, int dir)
    {
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
    std::tuple<float,float> PivotDriveController::get_best_effort_radius_direction(float target_radius, float target_direction) 
    {
        std::tuple<float,int,bool> best_efforts_left_right[2]; //array of {radius, direction, valid} for front left and front right pivots
        int drive_dir, best_dir, pivot_with_best_radius = 0;

        //iterate through each front pivot
        for (int i = 0; i < 2; i++) 
        {
           //calc angle for turning radius 
           double target_angle = get_pivot_angle_from_radius(target_radius, i && true, target_direction);

           float current_pivot_angle = i == 0 ? registered_left_pivot_handles_[0].state.get().get_value() : registered_right_pivot_handles_[0].state.get().get_value();
           
           //determine drive direction to reach target_angle
           if (current_pivot_angle < target_angle) {
               drive_dir = 1;
           } else {
               drive_dir = -1;
           }

           //calculate max. angle of pivot
           double best_effort_angle = current_pivot_angle + drive_dir * params_.max_d_theta;
           if (abs(current_pivot_angle - target_angle) < params_.max_d_theta)
           {
                best_effort_angle = target_angle;
           }

           //calculate direction
           if (current_pivot_angle == target_angle)
           {
                best_dir = 0;

           } else if (i == 0)
           {
                best_dir = best_effort_angle > angle_offset ? 1 : -1;
           } else
           {
                best_dir = best_effort_angle > angle_offset ? -1 : 1;
           }

           float best_radius = abs(get_radius_from_angle(best_effort_angle, i == 0));
           double left_angle = get_pivot_angle_from_radius(best_radius, true, best_dir);
           double right_angle = get_pivot_angle_from_radius(best_radius, false, best_dir);
           //bool valid = (abs(left_angle - curr_left) <= max_d_theta*1.01) && (abs(right_angle - curr_right) <= max_d_theta*1.01);
           bool valid = (abs(left_angle - registered_left_pivot_handles_[0].state.get().get_value()) <= params_.max_d_theta*1.01) &&
               (abs(right_angle - registered_right_pivot_handles_[0].state.get().get_value()) <= params_.max_d_theta * 1.01);

           best_efforts_left_right[i] = {best_radius, best_dir, valid};
        }

        
        //calculate which radius is 'best' to use
        pivot_with_best_radius = 0; //0 = left, 1 = right
        for (int i =0; i < 2; i++) {
            //if this configuration is invalid
            if (std::get<2>(best_efforts_left_right[i]) == 0) {
                return {std::get<0>(best_efforts_left_right[1-i]), std::get<1>(best_efforts_left_right[i-1])};
            }

            if (i == 1) {
                float radius_difference_1 = abs(std::get<0>(best_efforts_left_right[i])*std::get<1>(best_efforts_left_right[i]) - target_radius * target_direction);
                float radius_difference_0 = abs(std::get<0>(best_efforts_left_right[0])*std::get<1>(best_efforts_left_right[i]) - target_radius * target_direction);

                if (radius_difference_1 < radius_difference_0) {
                    pivot_with_best_radius = 1;
                }
            }
        }

        return {std::get<0>(best_efforts_left_right[pivot_with_best_radius]), std::get<1>(best_efforts_left_right[pivot_with_best_radius])};
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

        /*
        const double wheel_separation = params_.wheel_separation_multiplier * params_.wheel_separation;
        const double left_wheel_radius = params_.left_wheel_radius_multiplier * params_.wheel_radius;
        const double right_wheel_radius = params_.right_wheel_radius_multiplier * params_.wheel_radius;
        const double wheelbase = params_.wheelbase_multiplier * params_.wheelbase;
        const double max_d_theta = params_.max_d_theta;
        */

        //const double angle_offset = std::atan2(params_.steering_track, params_.wheel_base);

        /*
        odometry_.setWheelParams(params_.steering_track, params_.wheel_radius, params_.wheel_base, params_.wheel_steering_y_offset);
        odometry_.setVelocityRollingWindowSize(params_.velocity_rolling_window_size);
        */

        cmd_vel_timeout_ = std::chrono::milliseconds{static_cast<int>(params_.cmd_vel_timeout * 1000.0)};
        
        //publish_limited_drive_pivot_ = params_.publish_limited_drive_pivot;

        limiter_linear_ = SpeedLimiter(
            params_.has_velocity_limits, params_.has_acceleration_limits,
            params_.has_jerk_limits, params_.min_velocity, params_.max_velocity,
            params_.min_acceleration, params_.max_acceleration, params_.min_jerk,
            params_.max_jerk);

        /*
        //I'm assuming this would be some sort of PositionLimiter (if it exists)
        limiter_pivot_ = SpeedLimiter(
            params_.angular.z.has_velocity_limits, params_.angular.z.has_acceleration_limits,
            params_.angular.z.has_jerk_limits, params_.angular.z.min_velocity,
            params_.angular.z.max_velocity, params_.angular.z.min_acceleration,
            params_.angular.z.max_acceleration, params_.angular.z.min_jerk, params_.angular.z.max_jerk);
        */

        if (!reset())
        {
            return controller_interface::CallbackReturn::ERROR;
        }

        // left and right sides are both equal at this point
        //params_.wheels_per_side = params_.left_wheel_names.size();

        /*
        if (publish_limited_drive_pivot_)
        {
            limited_drive_pivot_publisher_ =
              get_node()->create_publisher<core::msg::DriveInputStamped>(DEFAULT_COMMAND_OUT_TOPIC, rclcpp::SystemDefaultsQoS());
            realtime_limited_drive_pivot_publisher_ =
              std::make_shared<realtime_tools::RealtimePublisher<core::msg::DriveInputStamped>>(limited_drive_pivot_publisher_);
        }
        */

        const core::msg::DriveInputStamped empty_drive_input;
        received_drive_input_msg_ptr_.set(std::make_shared<core::msg::DriveInputStamped>(empty_drive_input));

        /*
        const geometry_msgs::msg::Twist empty_twist;
        received_twist_msg_ptr_.set(std::make_shared<geometry_msgs::msg::Twist>(empty_twist));
        */

        RCLCPP_INFO(get_node()->get_logger(), "drive_input_msg_ptr");

        // Fill last two commands with default constructed commands
        previous_commands_.emplace(empty_drive_input);
        previous_commands_.emplace(empty_drive_input);

        RCLCPP_INFO(get_node()->get_logger(), "about to initialize subscriber");

        // initialize command subscriber
        drive_input_subscriber_ = get_node()->create_subscription<core::msg::DriveInputStamped>(
            DEFAULT_INPUT_TOPIC, rclcpp::SystemDefaultsQoS(),
            [this](const std::shared_ptr<core::msg::DriveInputStamped> msg) -> void
            {
              if (!subscriber_is_active_)
              {
                RCLCPP_WARN(get_node()->get_logger(), "Can't accept new commands. subscriber is inactive");
                return;
              }
              if ((msg->header.stamp.sec == 0) && (msg->header.stamp.nanosec == 0))
              {
                RCLCPP_WARN_ONCE(
                  get_node()->get_logger(),
                  "Received DriveInputStamped msg with zero timestamp, setting it to current "
                  "time, this message will only be shown once");
                msg->header.stamp = get_node()->get_clock()->now();
              }
              received_drive_input_msg_ptr_.set(std::move(msg));
            });

        RCLCPP_INFO(get_node()->get_logger(),"about to initiliase odometry publisher");

        // initialize odometry publisher and messasge
        /*
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
        odometry_message.twist = geometry_msgs::msg::TwistWithCovariance(rosidl_runtime_cpp::MessageInitialization::ALL);

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
        */

        previous_update_timestamp_ = get_node()->get_clock()->now();
        return controller_interface::CallbackReturn::SUCCESS;
    }

    controller_interface::CallbackReturn PivotDriveController::on_activate(
        const rclcpp_lifecycle::State &)
    {

        RCLCPP_INFO(get_node()->get_logger(),"on activate");
        const auto left_drives_result =
            configure_drive_pivots(true, params_.left_drive_names, registered_left_drive_handles_);
        const auto right_drives_result =
            configure_drive_pivots(true, params_.right_drive_names, registered_right_drive_handles_);
        const auto left_pivots_result =
            configure_drive_pivots(false, params_.left_pivot_names, registered_left_pivot_handles_);
        const auto right_pivots_result =
            configure_drive_pivots(false, params_.right_pivot_names, registered_right_pivot_handles_);

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
            RCLCPP_INFO(get_node()->get_logger(),"stuck here");

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
        std::swap(previous_commands_, empty);

        registered_left_drive_handles_.clear();
        registered_right_drive_handles_.clear();
        registered_left_pivot_handles_.clear();
        registered_right_pivot_handles_.clear();

        subscriber_is_active_ = false;
        drive_input_subscriber_.reset();

        RCLCPP_DEBUG(get_node()->get_logger(), "reset");

        received_drive_input_msg_ptr_.set(nullptr);
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
    }

    controller_interface::CallbackReturn PivotDriveController::configure_drive_pivots(
        bool drive, const std::vector<std::string> & wheel_names,
        std::vector<WheelHandle> & registered_handles)
    {
        //drive -- true or false
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
            const auto interface_name = feedback_type();
            const auto state_handle = std::find_if(
                state_interfaces_.cbegin(), state_interfaces_.cend(), //state_interfaces_ is handled by controller_manager (all available state interfaces)
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
                [&wheel_name, drive](const auto & interface)
                {
                if (drive) {
                    return interface.get_prefix_name() == wheel_name &&
                            interface.get_interface_name() == HW_IF_VELOCITY;
                } else {
                    return interface.get_prefix_name() == wheel_name &&
                            interface.get_interface_name() == HW_IF_POSITION;
                }
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
