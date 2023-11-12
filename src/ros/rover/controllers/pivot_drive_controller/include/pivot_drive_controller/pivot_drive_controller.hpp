//standard ros2_control stuff
#include "controller_interface/controller_interface.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_box.h"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"
#include "tf2_msgs/msg/tf_message.hpp"

//idk might need this
#include "pivot_drive_controller_parameters.hpp"

namespace pivot_drive_controller
{
    class PivotDriveController : public controller_interface:ControllerInterface
    {
        // (if param_.autonomous_mode){
        //     using CommandMsg = geometry_msgs::msg::Twist;
        // } else {
        //     using CommandMsg = core::msg::DriveInput;
        // }
        using DriveInput = core::msg::DriveInput;

    public:
        PIVOT_DRIVE_CONTROLLER_PUBLIC
        PivotDriveController();

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::InterfaceConfiguration command_interface_configuration() const override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::InterfaceConfiguration state_interface_configuration() const override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::return_type update(
            const rclcpp::Time & time, const rclcpp::Duration & period) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_init() override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_configure(
            const rclcpp_lifecycle::State & previous_state) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_activate(
            const rclcpp_lifecycle::State & previous_state) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_deactivate(
            const rclcpp_lifecycle::State & previous_state) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_cleanup(
            const rclcpp_lifecycle::State & previous_state) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_error(
            const rclcpp_lifecycle::State & previous_state) override;

        PIVOT_DRIVE_CONTROLLER_PUBLIC
        controller_interface::CallbackReturn on_shutdown(
            const rclcpp_lifecycle::State & previous_state) override;

    protected:
        struct WheelHandle
        {
            std::reference_wrapper<const hardware_interface::LoanedStateInterface> state;
            std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
        };

        const char * feedback_type() const;

        controller_interface::CallbackReturn configure_side(
            const std::string & side, const std::vector<std::string> & wheel_names,
            std::vector<WheelHandle> & registered_handles);

        std::vector<WheelHandle> registered_left_drive_handles_;
        std::vector<WheelModule> registered_right_drive_handles_;
        std::vector<WheelHandle> registered_left_pivot_handles_;
        std::vector<WheelHandle> registered_right_pivot_handles_;

        // Parameters from ROS for pivot_drive_controller
        std::shared_ptr<ParamListener> param_listener_;
        Params params_;

        Odometry odometry_;

        // Timeout to consider cmd_vel commands old
        std::chrono::milliseconds cmd_vel_timeout_{500};

        std::shared_ptr<rclcpp::Publisher<nav_msgs::msg::Odometry>> odometry_publisher_ = nullptr;
        std::shared_ptr<realtime_tools::RealtimePublisher<nav_msgs::msg::Odometry>>
        realtime_odometry_publisher_ = nullptr;

        std::shared_ptr<rclcpp::Publisher<tf2_msgs::msg::TFMessage>> odometry_transform_publisher_ =
        nullptr;
        std::shared_ptr<realtime_tools::RealtimePublisher<tf2_msgs::msg::TFMessage>>
        realtime_odometry_transform_publisher_ = nullptr;

        bool subscriber_is_active_ = false;
        rclcpp::Subscription<DriveInput>::SharedPtr drive_input_subscriber_ = nullptr;

        realtime_tools::RealtimeBox<std::shared_ptr<DriveInput>> received_drive_input_msg_ptr_{nullptr};

        std::queue<DriveInput> previous_commands_;  // last two commands

        // speed limiters
        SpeedLimiter limiter_linear_;

        bool publish_limited_drive_pivot_ = false;
        std::shared_ptr<rclcpp::Publisher<DriveInput>> limited_drive_pivot_publisher_ = nullptr;
        std::shared_ptr<realtime_tools::RealtimePublisher<DriveInput>> realtime_limited_drive_pivot_publisher_ =
        nullptr;

        rclcpp::Time previous_update_timestamp_{0};

        // publish rate limiter
        double publish_rate_ = 50.0;
        rclcpp::Duration publish_period_ = rclcpp::Duration::from_nanoseconds(0);
        rclcpp::Time previous_publish_timestamp_{0, 0, RCL_CLOCK_UNINITIALIZED};

        bool is_halted = false;

        bool reset();
        void halt();

