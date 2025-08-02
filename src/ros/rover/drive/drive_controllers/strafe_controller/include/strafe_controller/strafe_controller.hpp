#ifndef STRAFE_CONTROLLER__STRAFE_CONTROLLER_HPP_
#define STRAFE_CONTROLLER__STRAFE_CONTROLLER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <tuple>

#include "hardware_interface/handle.hpp"
#include "controller_interface/controller_interface.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "std_srvs/srv/empty.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_box.h"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"

#include "nova_interfaces/msg/drive_input_stamped.hpp"
#include "nova_controller_common/speed_limiter.hpp"
#include "strafe_controller/odometry.hpp"
#include "strafe_controller_parameters.hpp"

namespace strafe_controller
{
    class StrafeController : public controller_interface::ControllerInterface
    {

    public:
        StrafeController();

        controller_interface::InterfaceConfiguration command_interface_configuration() const override;
        controller_interface::InterfaceConfiguration state_interface_configuration() const override;

        controller_interface::return_type update(
            const rclcpp::Time &time, const rclcpp::Duration &period) override;

        controller_interface::CallbackReturn on_init() override;

        controller_interface::CallbackReturn on_configure(
            const rclcpp_lifecycle::State &previous_state) override;

        controller_interface::CallbackReturn on_activate(
            const rclcpp_lifecycle::State &previous_state) override;

        controller_interface::CallbackReturn on_deactivate(
            const rclcpp_lifecycle::State &previous_state) override;

        controller_interface::CallbackReturn on_cleanup(
            const rclcpp_lifecycle::State &previous_state) override;

        controller_interface::CallbackReturn on_error(
            const rclcpp_lifecycle::State &previous_state) override;

        controller_interface::CallbackReturn on_shutdown(
            const rclcpp_lifecycle::State &previous_state) override;

    protected:
        struct WheelHandle
        {
            std::reference_wrapper<const hardware_interface::LoanedStateInterface> state;
            std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
        };

        const char *drive_feedback_type() const;
        const char *pivot_feedback_type() const;

        controller_interface::CallbackReturn configure_drive_pivots(
            const std::vector<std::string> &wheel_names,
            std::vector<WheelHandle> &registered_handles, const char *feedback_type);

        std::vector<WheelHandle> registered_left_drive_handles_;
        std::vector<WheelHandle> registered_right_drive_handles_;
        std::vector<WheelHandle> registered_left_pivot_handles_;
        std::vector<WheelHandle> registered_right_pivot_handles_;

        // Parameters from ROS for strafe_controller
        std::shared_ptr<ParamListener> param_listener_;
        Params params_;

        Odometry odometry_;

        // Timeout to consider cmd_vel commands old
        std::chrono::milliseconds cmd_vel_timeout_{500};

        std::shared_ptr<rclcpp::Publisher<nav_msgs::msg::Odometry>> odometry_publisher_ = nullptr;
        std::shared_ptr<realtime_tools::RealtimePublisher<nav_msgs::msg::Odometry>>
            realtime_odometry_publisher_ = nullptr;

        std::shared_ptr<rclcpp::Publisher<tf2_msgs::msg::TFMessage>> odometry_transform_publisher_ = nullptr;
        std::shared_ptr<realtime_tools::RealtimePublisher<tf2_msgs::msg::TFMessage>>
            realtime_odometry_transform_publisher_ = nullptr;

        bool subscriber_is_active_ = false;
        rclcpp::Subscription<nova_interfaces::msg::DriveInputStamped>::SharedPtr drive_input_subscriber_ = nullptr;
        rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr twist_subscriber_ = nullptr;

        realtime_tools::RealtimeBox<std::shared_ptr<nova_interfaces::msg::DriveInputStamped>> received_drive_input_msg_ptr_{nullptr};
        realtime_tools::RealtimeBox<std::shared_ptr<geometry_msgs::msg::TwistStamped>> received_twist_msg_ptr_{nullptr};

        std::queue<nova_interfaces::msg::DriveInputStamped> previous_commands_; // last two commands
        std::queue<geometry_msgs::msg::TwistStamped> previous_twist_commands_;  // last two commands

        // speed limiters
        nova_controller_common::SpeedLimiter limiter_linear_;

        float angle_offset = params_.steering_track / params_.wheel_base;
        float best_effort_velocity = 0.0;

        double max_d_theta;
        double max_d_vel;

        rclcpp::Time previous_update_timestamp_{0};

        // publish rate limiter
        double publish_rate_ = 50.0;
        rclcpp::Duration publish_period_ = rclcpp::Duration::from_nanoseconds(0);
        rclcpp::Time previous_publish_timestamp_{0, 0, RCL_CLOCK_UNINITIALIZED};

        bool is_halted = false;

        bool reset();
        void halt();
    };
} // namespace strafe_controller
#endif // STRAFE_CONTROLLER__STRAFE_CONTROLLER_HPP_
