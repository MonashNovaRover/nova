#ifndef PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_
#define PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <tuple>

#include "controller_interface/controller_interface.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "std_srvs/srv/empty.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "hardware_interface/handle.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_box.h"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"

#include "nova_interfaces/msg/drive_input_stamped.hpp"
#include "pivot_drive_controller/odometry.hpp"
#include "nova_controller_common/speed_limiter.hpp"
#include "pivot_drive_controller/visibility_control.h"
#include "pivot_drive_controller_parameters.hpp"

namespace pivot_drive_controller
{
    class PivotDriveController : public controller_interface::ControllerInterface
    {

    public:
        PIVOT_DRIVE_CONTROLLER_PUBLIC
        PivotDriveController();

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::InterfaceConfiguration command_interface_configuration() const override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::InterfaceConfiguration state_interface_configuration() const override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::return_type update(
            const rclcpp::Time &time, const rclcpp::Duration &period) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_init() override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_configure(
            const rclcpp_lifecycle::State &previous_state) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_activate(
            const rclcpp_lifecycle::State &previous_state) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_deactivate(
            const rclcpp_lifecycle::State &previous_state) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_cleanup(
            const rclcpp_lifecycle::State &previous_state) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_error(
            const rclcpp_lifecycle::State &previous_state) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
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

        double get_pivot_angle_from_radius(float radius, bool left, int dir);

        std::tuple<float, int> get_best_effort_radius_direction(float radius, float dir);

        double get_radius_from_angle(double angle, bool left);

        std::vector<WheelHandle> registered_left_drive_handles_;
        std::vector<WheelHandle> registered_right_drive_handles_;
        std::vector<WheelHandle> registered_left_pivot_handles_;
        std::vector<WheelHandle> registered_right_pivot_handles_;

        // Parameters from ROS for pivot_drive_controller
        std::shared_ptr<ParamListener> param_listener_;
        Params params_;

        Odometry odometry_;

        double zero_radius_;

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
        rclcpp::Subscription<nova_interfaces::msg::DriveInputStamped>::SharedPtr drive_input_unstamped_subscriber_ = nullptr;
        rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr twist_unstamped_subscriber_ = nullptr;

        realtime_tools::RealtimeBox<std::shared_ptr<nova_interfaces::msg::DriveInputStamped>> received_drive_input_msg_ptr_{nullptr};
        realtime_tools::RealtimeBox<std::shared_ptr<geometry_msgs::msg::TwistStamped>> received_twist_msg_ptr_{nullptr};

        std::queue<nova_interfaces::msg::DriveInputStamped> previous_commands_; // last two commands
        std::queue<geometry_msgs::msg::TwistStamped> previous_twist_commands_;  // last two commands

        // speed limiters
        SpeedLimiter limiter_linear_;

        double angle_offset_ = params_.steering_track / params_.wheel_base;

        /*
        bool publish_limited_drive_pivot_ = false;
        std::shared_ptr<rclcpp::Publisher<CommandMsg>> limited_drive_pivot_publisher_ = nullptr;
        std::shared_ptr<realtime_tools::RealtimePublisher<CommandMsg>> realtime_limited_drive_pivot_publisher_ = nullptr;
        */

        double max_d_theta;

        double prev_radius = INFINITY;
        double prev_dir = 0;

        rclcpp::Time previous_update_timestamp_{0};

        // publish rate limiter
        double publish_rate_ = 50.0;
        rclcpp::Duration publish_period_ = rclcpp::Duration::from_nanoseconds(0);
        rclcpp::Time previous_publish_timestamp_{0, 0, RCL_CLOCK_UNINITIALIZED};

        bool is_halted = false;

        bool reset();
        void halt();
    };
} // namespace pivot_drive_controller
#endif // PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_
