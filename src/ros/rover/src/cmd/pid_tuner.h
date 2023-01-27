#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class is able to interface with the CMDs and
    tune each of the motors with PID constants.
It also publishes data from the wheels based on the
    selected device.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: pid_tuner
TOPICS:
  - /control/cmd_feedback   [CMDFeedback]   [Published]
SERVICES:
  - /control/pid_tune       [PIDTune]       [Service]
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
CREATION:	05/01/2022
EDITED:		06/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include required ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/cmd_feedback.hpp"
#include "core/srv/pid_tune.hpp"
#include <memory>

// Include CMD class
#include "cmd.h"

// Use the standard namespaces
using namespace std::chrono_literals;
using std::placeholders::_1;
using std::placeholders::_2;


// The PID Tuner class
class PIDTuner : public rclcpp::Node {

    // The number of wheels on the rover
    static const int NUM_WHEELS = 6;

    // The number of devices on the arm
    static const int NUM_ARM_DEVICES = 6;


    //------------------------------------------------------------//
    private:

    // Stores the loop timer for the velocity function
    rclcpp::TimerBase::SharedPtr velocity_timer;

    // Stores the loop timer for the feedback function
    rclcpp::TimerBase::SharedPtr feedback_timer;

    // Stores the publisher for the CMD feedbcak
    rclcpp::Publisher<core::msg::CMDFeedback>::SharedPtr publisher;

    // Stores the service for the PID commands
    rclcpp::Service<core::srv::PIDTune>::SharedPtr service;

    // Stores the arrays of CMDs for each bus
    CMD* bus_0 [NUM_WHEELS];
    CMD* bus_1 [NUM_ARM_DEVICES];

    // A flag for whether a device has been selected
    bool valid;

    // The current selected bus
    int bus;

    // The current selected id
    int id;

    // The current velocity to send
    double velocity;

    // A flag for whether to send zeroes
    bool stopped;



    //------------------------------------------------------------//
    private:

    /// @brief      Called when a client message is received and selects a device
    /// @param      request - A reference to the request portion of the message
    /// @param      response - A reference to the response portion of the message
    void select_device (
        const core::srv::PIDTune::Request::SharedPtr request,
        core::srv::PIDTune::Response::SharedPtr response);

    /// @brief      Updates the constants of the selected device
    /// @param      kP - The Proportionality constant
    /// @param      kI - The Intergral constant
    /// @param      kD - The Differential constant
    /// @param      kM - The Midpoint interval
    void update_constants (double kP = 0.0, double kI = 0.0, 
        double kD = 0.0, double kM = 0.0);

    /// @brief      Returns the currently selected CMD
    /// @returns    A pointer to the CMD selected
    CMD* get_cmd ();

    /// @brief      Sends the velocities to the selected CMD
    void send_velocity ();

    /// @brief      Publishes feedback from the selected CMD
    void publish_velocity ();


    //------------------------------------------------------------//
    public:

    /// @brief      Default constructor function that starts up the node
    PIDTuner();

};