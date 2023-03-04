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
#include "core/msg/input_gamepad.hpp"
#include "core/msg/drive_input.hpp"
#include "core/msg/telemetry.hpp"
#include "core/msg/single_telemetry.hpp"
#include "core/msg/pivot_wheel_data.hpp"
#include "std_msgs/msg/bool.hpp"

// Include other headers
#include<cmath>

// Include CMD class
#include "cmd/blcmd.h"

// The distance between left and right wheels [m]
#define CHASSIS_WIDTH 0.7

// The distance between front and rear wheels [m]
#define CHASSIS_LENGTH 0.84

#define MAX_RADIUS 10.0

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
    PivotModule(int id, BLCMD *cmdWheel, BLCMD *cmdPivot)
    {
        this->id = id;
        this->cmdWheel = cmdWheel;
        this->cmdPivot = cmdPivot;
        this->angle = 0.0;
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

    const float alpha = 0.0;
    float steer = 0.0;
    const float angle_offset = atan((CHASSIS_WIDTH)/CHASSIS_LENGTH);

    // A flag for whether to use autonomous state or not
    bool is_autonomous = false;

    // An array of pointers to Wheel instances
    PivotModule *pivots[NUM_WHEELS];
private:
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

    void blcmd_spinner();

    /// @brief     Calculates the turning radius of the rover
    /// @param      steer
    /// @returns
    float get_turning_radius(float steer);

    /// @brief
    /// @param      radius - The turning radius of the rover [m]
    void fill_wheel_angles_radial(float radius, float steer);

    /// @brief
    /// @param      speed - Speed of each driven wheel
    /// @param      steer - Direction and amount of steering
    void fill_wheel_velocities_radial(float speed, float steer);

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
