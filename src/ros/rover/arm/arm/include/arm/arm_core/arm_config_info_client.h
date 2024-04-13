#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class implements a client for /arm/arm_config_info
  and saves the relevant config information
Other nodes can inherit this class to have access to this info
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES:
  - /arm/arm_config_info     [arm_interfaces/ArmConfigInfo]        [Client]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 28/04/2022
EDITED:		 28/04/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include ROS client library
#include "rclcpp/rclcpp.hpp"
// Include service types
#include "arm_interfaces/srv/arm_config_info.hpp"

// Include other libraries
#include <string>

/*
Class which gets arm configuration information from the /arm/arm_config_info service
*/
class ArmConfigInfoClient : public rclcpp::Node
{
    //------------------------------------------------------------//
    private:

    // Timer to check for responses from the service
    rclcpp::TimerBase::SharedPtr check_receive_timer;
    
    // Service client for /arm/arm_config_info and the associated future to check later
    rclcpp::Client<arm_interfaces::srv::ArmConfigInfo>::SharedPtr client;
    std::shared_future<arm_interfaces::srv::ArmConfigInfo::Response::SharedPtr> future;

    /// @brief    Callback to check if the service has responded
    ///           If received a response, cancels the timer and starts the child node
    void check_receive_callback();
    
    //------------------------------------------------------------//
    protected:

    // Store response from /arm/arm_config_info
    // Make it protected so child classes can access
    arm_interfaces::srv::ArmConfigInfo::Response arm_config_info;

    /// @brief    Abstract function to be called once the clinet gets a response
    ///           Implemented by the child class to add any additional publihsers,
    ///           subscribers and other node-specific construction logic.
    virtual void start_node() = 0;

    //------------------------------------------------------------//
    public:

    /// @brief    Constructor to start the node and make the service request.
    ///           Initialises the node with the name given by the child class
    ArmConfigInfoClient(const std::string& node_name);
};