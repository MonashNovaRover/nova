/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Jory Braun
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_core/arm_config_info_client.h"
#include "colors.h"
#include "rosconfig.h"

ArmConfigInfoClient::ArmConfigInfoClient(const std::string& node_name) : Node(node_name)
{
    // Create the service client for /arm/arm_config_info
    client = this->create_client<arm_interfaces::srv::ArmConfigInfo>("/arm/arm_config_info");
    // Wait for the service to become available
    while (!client->wait_for_service(1s)){
        RCLCPP_INFO(this->get_logger(), "Service /arm/arm_config_info not available, waiting again...");
    }
    // Make the service request
    auto request = std::make_shared<arm_interfaces::srv::ArmConfigInfo::Request>();
    future = client->async_send_request(request);

    // Set up the callback timer
    check_receive_timer = this->create_wall_timer(
        ROSTimers::arm_startup_timer, std::bind(&ArmConfigInfoClient::check_receive_callback, this)
    );

    // Output set-up messages
    // Format node title
    std::string node_name_upper = node_name;
    for (auto& c : node_name_upper){
        c = (c == '_') ? ' ' : toupper(c);
    }
    std::cout << C_TITLE << "ARM CONFIG INFO CLIENT: " << node_name_upper << C_END << "\n";
    std::cout << "Service Clients:\n";
    std::cout << "/arm/arm_config_info    [arm_interfaces/ArmConfigInfo]\n" << std::endl;
}

// Check if the service has responded, if so, start the child node
void ArmConfigInfoClient::check_receive_callback()
{
    // Check if the service has responded
    if (future.wait_for(1s) == std::future_status::ready) {
        // Got a response!
        RCLCPP_INFO(this->get_logger(), "Got a response from /arm/arm_config_info. Starting the node.");
        // Cancel the timer
        check_receive_timer->cancel();
        // Get the data
        arm_config_info = *(future.get());
        // Start the node
        start_node();
    }
    else{
        RCLCPP_INFO(this->get_logger(), "Failed to get response from /arm/arm_config_info, waiting again...");
    }
}
