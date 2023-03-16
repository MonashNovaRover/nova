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

// Include custom ROS messages
#include "core/msg/input_gamepad.hpp"
#include "core/msg/drive_input.hpp"
#include "core/msg/telemetry.hpp"
#include "core/msg/single_telemetry.hpp"
#include "core/msg/pivot_wheel_data.hpp"
#include "core/msg/blcmd_status_array.hpp"
#include "core/msg/blcmd_status.hpp"


// Include other headers
#include<cmath>
#include<vector>

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

class PivotModule
{
public:
    int id;
    double velocity;
    double angle;
    BLCMD *cmdWheel;
    BLCMD *cmdPivot;

    ///@brief   contructor for PivotModule
    ///@param    id - the id of the module
    ///@param   cmdWheel - BLCMD for the wheel
    ///@param   cmdPivot - BLCMD for the pivot
    PivotModule(int id, BLCMD *cmdWheel, BLCMD *cmdPivot, float angle)
    {
        this->id = id;
        this->cmdWheel = cmdWheel;
        this->cmdPivot = cmdPivot;
        this->angle = angle;
        this->velocity = 0.0;
    }
};

// Main subscriber class that receives drives commands and interfaces with the wheel
class Driver : public rclcpp::Node
{

    // The number of wheels on the rover
    static const int NUM_WHEELS = 4;

private:
    // Stores the subscriber for the drive commands (manual)
    rclcpp::Subscription<core::msg::DriveInput>::SharedPtr subscription_cmds_man;

    // Stores the subscriber for the drive commands (auto)
    rclcpp::Subscription<core::msg::DriveInput>::SharedPtr subscription_cmds_auto;

    // Stores the subscriber to the gamepad inputs
    rclcpp::Subscription<core::msg::InputGamepad>::SharedPtr subscription_inputs;

    // Stores the subscriber to the BLCMD status
    rclcpp::Subscription<core::msg::BLCMDStatusArray>::SharedPtr subscription_blcmd_status;

    // Publishes whether the rover is in autonomous mode for LEDs
    rclcpp::TimerBase::SharedPtr mode_timer;
    rclcpp::TimerBase::SharedPtr telemetry_timer;
    rclcpp::TimerBase::SharedPtr blcmd_spin_timer;

    rclcpp::Publisher<std_msgs::msg::Bool>::SharedPtr mode_pub;
    rclcpp::Publisher<core::msg::Telemetry>::SharedPtr telemetry_pub;

    // Publisher for pivot wheel data
    rclcpp::Publisher<core::msg::PivotWheelData>::SharedPtr pivot_wheel_pub;

    // A flag for whether to apply the handbrake or not
    bool handbrake;

    // A flag for weather a blcmd has an error
    bool blcmd_error = false;

    // The absolute value of the offset between the blcmds 0 and 0 on the rover
    const float angle_offset = atan((CHASSIS_WIDTH)/CHASSIS_LENGTH);

    // the previous sign of the turning radius
    int sign = 0;

    // A flag for whether to use autonomous state or not
    bool is_autonomous = false;
    double d_theta;

    // An array of pointers to Wheel instances
    PivotModule *pivots[NUM_WHEELS];

    /// @brief      Sends commands to the wheels using the wheel classes
    /// @param      msg - A pointer to the drive message
    void send_commands(const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Callback function when drive messages are received
    /// @param      msg - A pointer to the drive message
    void drive_callback(const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Callback function when autonomous messages are received
    /// @param      msg - A pointer to the drive message
    void auto_callback(const core::msg::DriveInput::SharedPtr msg);

    /// @brief      Callback function when input messages are received.
    /// @param      msg - A pointer to the input message
    void input_callback(const core::msg::InputGamepad::SharedPtr msg);

    /// @brief      Callback function when BLCMD status messages are received.
    /// @param      msg - A pointer to the BLCMD status message
    void blcmd_status_callback(const core::msg::BLCMDStatusArray::SharedPtr msg);

    /// @brief      function that spins all blcmds
    void blcmd_spinner();

    /// @brief      calculates the angle of a wheel
    /// @param      radius - the radius of the turn
    /// @param      wheel - the wheel number (0-4 CCW)
    /// @param      sign - the sign of the radius
    /// @returns    the angle of the wheel
    double calc_wheel_angle(float radius, int wheel, int sign);

    /// @brief      calculates the radius from the angle of one wheel (inverse of calc_wheel_angle)
    /// @param      angle - the angle of the wheel
    /// @param      wheel - the wheel number (0-4 CCW)
    /// @param      sign - the sign of the radius
    /// @returns    the radius of the turn
    double radius_from_angle(double angle, int wheel, int sign);

    /// @brief     Calculates the radius to turn the wheels to such that the radius is valid based on the maximum
    ///            pivot angular velocity
    /// @param     steer - The value of steer between -1 and 1 where -1 is on the spot turn left and 1 is on the spot turn right
    /// @returns   the radius to turn the wheels to
    double get_turning_radius(float steer);

    /// @brief      fills the wheel angles of the pivots array based on a radius
    /// @param      radius - The turning radius of the rover [m]
    void fill_wheel_angles_radial(double radius);

    /// @brief      fills the wheel angles of the pivots array when strafe mode is enabled
    void fill_wheel_angles_strafe();

    /// @brief      fills the wheel velocities of the pivots array based on a radius
    /// @param      speed - Speed of each driven wheel
    /// @param      steer - Direction and amount of steering
    void fill_wheel_velocities_radial(float speed, float radius);

    /// @brief      fills the wheel velocities of the pivots array when strafe mode is enabled
    /// @param      speed - Speed of each driven wheel
    void fill_wheel_velocities_strafe(float speed);

    /// @brief      Callback function to publish whether autonomous
    void pub_auto_mode();

    /// @brief      Callback function to publish telemetry data
    void pub_telemetry();

    /// @brief callback for when drive inputs subscription is exceeded
    void inputs_deadline_exceeded();
    //------------------------------------------------------------//
public:
    /// @brief      Default constructor function that starts up the node
    Driver();
};
