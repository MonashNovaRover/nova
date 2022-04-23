#include "spm_pid.hpp"

SpmWristDriver::SpmWristDriver(int m0, int m1, int m2) {
    // assigns the joint index to the motors
    motor_joint_index[0] = m0;
    motor_joint_index[1] = m1;
    motor_joint_index[2] = m2;

    previous_pitch = 0.0;
    previous_yaw = 0.0;
}

void SpmWristDriver::update_goal(float yaw, float pitch, float roll) {
    // Update the goal of the PID loop
    previous_pitch = pitch;
    previous_yaw = yaw;
    motor_positions = wrist.motor_pos(yaw, pitch, roll);
}

std::vector<double>& SpmWristDriver::pid(rclcpp::Time current, sensor_msgs::msg::JointState state) {
    // update times
    rclcpp::Duration delta_time = current - last_pid_time;
    last_pid_time = current;

    for (unsigned int index = 0; index < response.size(); index++) {
        // response is proportional to error over timestep
        response[index] = kp * (state.position[motor_joint_index[index]] - motor_positions[index]) / delta_time.seconds();
    }
    return response;
}

std::vector<double>& SpmWristDriver::integrator(float pitch_velocity, float yaw_velocity) {
    float pitch  = previous_pitch + pitch_velocity * delta_time;
    float yaw = previous_yaw + yaw_velocity * delta_time;
    update_goal(yaw, pitch, M_PI/6.0);
}



