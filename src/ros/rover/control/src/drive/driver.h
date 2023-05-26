#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class interfaces with the wheels and ROS and
    is able to publish data over CAN to the wheels.
It can also read autonomous messages if the velocity
    convert script is running.
The wheels are indexed with 0, 1, 2 on the left and
    3, 4, 5 on the right, with the largest at the back.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: drive_sub
TOPICS:
  - /control/input_gamepad  [InputGamepad]  [Subscribed]
  - /control/drive_cmds     [DriveCmd]      [Subscribed]
  - /autonomous/drive_cmds  [DriveCmd]      [Subscribed]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):  Taaj Street, Harrison Verrios, Josh Cherubino,
            Will de la Rue, Jory Braun
CREATION:	21/11/2021
EDITED:		13/09/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS packages
#include "rclcpp/rclcpp.hpp"

// Include standard ROS messages
#include "std_msgs/msg/bool.hpp"
#include "std_msgs/msg/byte.hpp"

// Include custom ROS messages
#include "core/msg/input_gamepad.hpp"
#include "core/msg/drive_input.hpp"
#include "core/msg/telemetry.hpp"
#include "core/msg/single_telemetry.hpp"
#include "core/msg/pivot_wheel_data.hpp"
#include "core/msg/blcmd_status_array.hpp"
#include "core/msg/blcmd_status.hpp"

// Include custom ROS services
#include "core/srv/disable_blcmd.hpp"

// Include other headers
#include<cmath>
#include<vector>
#include<chrono>
#include<tuple>

// Include CMD class
#include "cmd/blcmd.h"

// The distance between left and right wheels [m]
#define CHASSIS_WIDTH 0.7

// The distance between front and rear wheels [m]
#define CHASSIS_LENGTH 0.84

// Use the standard namespaces
using namespace std;
using namespace std::chrono_literals;
using std::placeholders::_1;

struct Vector2 {

public:

    // The position values
    float x, y;

    // Constructor for the vector
    Vector2 (float x, float y) {
        this->x = x;
        this->y = y;
    }
};

class PivotModule
{
public:
    int id;
    bool pivot_enabled;
    bool wheel_enabled;
    double velocity;
    double angle;
    BLCMD *cmdWheel;
    BLCMD *cmdPivot;

    ///@brief   contructor for PivotModule
    ///@param    id - the id of the module
    ///@param   cmdWheel - BLCMD for the wheel
    ///@param   cmdPivot - BLCMD for the pivot
    PivotModule(int id, BLCMD *cmdWheel, BLCMD *cmdPivot, float angle);

    ///@brief   drives the pivot angle if enabled
    void drive_pivot();

    ///@brief   drives the wheel if enabled
    void drive_wheel();
};

// Main subscriber class that receives drives commands and interfaces with the wheel
class Driver : public rclcpp::Node
{

    // The number of wheels on the rover
    static const int NUM_WHEELS = 4;
    // The absolute value of the offset between the blcmds 0 and 0 on the rover
    const float angle_offset = atan((CHASSIS_WIDTH)/CHASSIS_LENGTH);
private:
    // Stores the subscriber for the drive commands (manual)
    rclcpp::Subscription<core::msg::DriveInput>::SharedPtr subscription_cmds;

    rclcpp::TimerBase::SharedPtr telemetry_timer;
    rclcpp::TimerBase::SharedPtr blcmd_spin_timer;
    rclcpp::TimerBase::SharedPtr send_commands_timer;

    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr mode_pub;
    rclcpp::Publisher<core::msg::Telemetry>::SharedPtr telemetry_pub;

    // Publisher for pivot wheel data
    rclcpp::Publisher<core::msg::PivotWheelData>::SharedPtr pivot_wheel_pub;

    // blcmd disable service
    rclcpp::Service<core::srv::DisableBLCMD>::SharedPtr disable_blcmd_srv;

    double max_d_theta;
    double max_d_vel;

    // Default control data
    float target_radius = INFINITY;
    float best_effort_radius = INFINITY;
    float target_velocity = 0.0;
    float best_effort_velocity = 0.0;
    int8_t target_direction = 0;
    int8_t best_effort_direction = 0;
    unsigned char mode = core::msg::DriveInput::TANK;
    bool handbrake;

    // An array of pointers to Wheel instances
    PivotModule *pivots[NUM_WHEELS];

    /// @brief      Sends commands to the blcmds using the PivotModule classes
    void send_commands();

    /// @brief      Callback function when drive messages are received
    /// @param      msg - A pointer to the drive message
    void drive_callback(const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Disable BLCMD service callback
    void disable_blcmd_callback(const std::shared_ptr<core::srv::DisableBLCMD::Request> request,
                                std::shared_ptr<core::srv::DisableBLCMD::Response> response);

    /// @brief      function that spins all blcmds
    void blcmd_spinner();

    /// @brief      calculates the angle of a wheel
    /// @param      radius - the radius of the turn
    /// @param      left - the wheel side, left is true
    /// @param      dir - the direction of the radius, -1 left turn, 1 right turn
    /// @returns    the angle of the wheel
    double calc_wheel_angle(float radius, bool left, int dir);

    /// @brief      calculates the radius from the angle of one wheel (inverse of calc_wheel_angle)
    /// @param      angle - the angle of the wheel
    /// @param      left - the wheel side, left is true
    /// @returns    the radius of the turn
    double radius_from_angle(double angle, bool left);

    /// @brief     calculates the best_effort_velocity based on the target_velocity and the max_d_vel
    void set_best_effort_velocity();

    /// @brief     Calculates the radius to turn the wheels to such that the radius is valid based on the maximum
    ///            pivot angular velocity
    void set_best_effort_radius();

    /// @brief      Calculates the radius when the when wheel is moved in the direction of the target radius
    ///             by the max_d_theta amount.
    /// @param      curr_left - the current angle of the left wheels
    /// @param      curr_right - the current angle of the right wheels
    /// @param      radius - the target radius
    /// @param      dir - the direction of the turn
    /// @param      left - calculate the left radius, else calculate the right radius
    /// @returns    a tuple containing the best effort radius, direction and if the radius is valid
    tuple<float, int, bool> calc_best_effort_radius(float curr_left, float curr_right, float radius, int dir, bool left);

    /// @brief      fills the wheel angles of the pivots array based on a radius
    void fill_wheel_angles_radial();

    /// @brief      fills the wheel angles of the pivots array when strafe mode is enabled
    void fill_wheel_angles_strafe();

    /// @brief      fills the wheel velocities of the pivots array based on a radius
    void fill_wheel_velocities_radial();

    /// @brief      fills the wheel velocities of the pivots array when strafe mode is enabled
    void fill_wheel_velocities_strafe();

    /// @brief      fills the wheel velocities of the pivots array when tank mode is enabled
    void fill_wheel_velocities_tank();

    /// @brief      Calculates the position of the wheel in relation to the wheelbase centre
    /// @param      id - The identification of the wheel
    /// @returns    The position vector (x, y)
    Vector2 get_wheel_position (int id);

    /// @brief      Calculates the distance from the wheel to the turning centre
    /// @param      pos - The position of the wheel
    /// @param      radius - The turning radius of the rover [m]
    /// @returns    The distance between wheel and the turning centre [m]
    float get_wheel_distance (Vector2 pos, float radius);

    /// @brief      Callback function to publish whether autonomous
    void pub_auto_mode();

    /// @brief      Callback function to publish telemetry data
    void pub_telemetry();

    /// @brief callback for when drive inputs subscription is exceeded
    void drive_inputs_deadline_exceeded();
    //------------------------------------------------------------//
public:
    /// @brief      Default constructor function that starts up the node
    Driver();
};
