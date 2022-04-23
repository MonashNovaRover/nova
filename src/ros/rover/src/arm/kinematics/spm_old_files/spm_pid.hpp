#include "spm.hpp"
#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/joint_state.hpp"

class SpmWristDriver
{
    private:
        SpmWrist wrist;
        // List of indexes for each motor in JointState
        int motor_joint_index[3];
        // Position of motor aim
        std::vector<double> response(3);

        rclcpp::Time last_pid_time;
        rclcpp::Time last_integration_time;
        double error_integrated[3] = { 0 };

        // PID coefficient
        float kp;

        double previous_pitch;
        double previous_yaw;

    public:
        SpmWristDriver(int m0, int m1, int m2);
        void update_goal(float yaw, float pitch, float roll);
        std::vector<double>& pid(rclcpp::Time current, sensor_msgs::msg::JointState state);
};