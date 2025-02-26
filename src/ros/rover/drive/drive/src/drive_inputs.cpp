/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	drive
AUTHOR(S):	Harrison Verrios, Liam Whittle, Taaj Street
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "drive/drive_inputs.h"
#include "colors.h"
#include "drive/drive_timers.h"
#include <math.h>

using std::placeholders::_1;

// Adjustes the multiplier factor by some amount in some direction
float DriveInputs::adjust_multiplier(float &multiplier, bool increase, bool coarse)
{
   float old_multiplier = multiplier;
    // Adjust the multiplier
    multiplier += (coarse ? DELTA_MULTIPLIER_COARSE : DELTA_MULTIPLIER_FINE) * (increase ? 1 : -1);
    // Check for minimum and maximums
    if (multiplier > MAX_MULTIPLIER)
        multiplier = MAX_MULTIPLIER;
    else if (multiplier <= (coarse ? MIN_COARSE_MULTIPLIER : MIN_FINE_MULTIPLIER))
        multiplier = coarse ? MIN_COARSE_MULTIPLIER : MIN_FINE_MULTIPLIER;

    if (!increase && multiplier > old_multiplier) {
        multiplier = old_multiplier;
    }
    // Return the new multiplier
    return multiplier;
}

// Publishes the drive commands from the speed and steer
void DriveInputs::publish_cmds()
{
    if (prev_msg_received) {
        // Publish the drive command
        drive_publisher->publish(latest_drive_input);

        // Clear the msg_received flag
        prev_msg_received = false;
    }
}

void DriveInputs::publish_info()
{
    // Create the info message
    auto info_msg = drive_interfaces::msg::DriveInfo();
    info_msg.multiplier = multiplier_speed;
    info_msg.locked = locked;
    info_msg.autonomous_mode = autonomous;
    info_msg.connected = connected;
    info_msg.drive_mode = latest_drive_input.mode;
    info_msg.handbrake = latest_drive_input.handbrake;
    // Publish the info
    info_publisher->publish(info_msg);
}

// Stops driving when no input received from radios for a period of time
void DriveInputs::input_deadline_exceeded()
{
    if (!autonomous) {
        // Clear the old inputs
        latest_drive_input.speed = 0.0;
        RCLCPP_WARN(this->get_logger(), "Input gamepad subscriber deadline missed");
        prev_msg_received = false;
    }
}

void DriveInputs::auto_deadline_exceeded() {
    if (autonomous) {
        latest_drive_input.speed = 0.0;
        prev_msg_received = false;
        RCLCPP_WARN(this->get_logger(), "Autonomous input subscriber deadline missed");
    }
}

void DriveInputs::autonomous_callback(const drive_interfaces::msg::DriveInput::SharedPtr msg) {
    if (autonomous){
        latest_drive_input.mode = msg->mode;
        latest_drive_input.speed = msg->speed;
        latest_drive_input.radius = msg->radius;
        latest_drive_input.direction = msg->direction;

        prev_msg_received = true;
    }
}

// Receives input from the gamepad
void DriveInputs::input_callback(const input_interfaces::msg::InputGamepad::SharedPtr msg)
{
    if (!autonomous)
        prev_msg_received = true;

    if (!msg->connected)
    {
        latest_drive_input.radius = INFINITY;
        latest_drive_input.direction = 0;
        latest_drive_input.speed = 0.0;
        // Publish no connection message
        if (connected)
            RCLCPP_INFO_STREAM(this->get_logger(), C_FAIL << "No Gamepad Connected" << C_END);
    } else {
        if (!connected)
            RCLCPP_INFO_STREAM(this->get_logger(), C_SUCCESS << "Gamepad Connected" << C_END);
    }
    connected = msg->connected;

    if (msg->btn_back_state == 1)
    {
        if (!locked)
            RCLCPP_INFO_STREAM(this->get_logger(), "Gamepad Locked");
        locked = true;
    }
    if (msg->btn_start_state == 1)
    {
        if (locked)
            RCLCPP_INFO_STREAM(this->get_logger(), "Gamepad Unlocked");
        locked = false;
    }

    if (locked && !autonomous) {
        latest_drive_input.speed = 0.0;
    }
    // Determine if the conrroller needs to be locked or not
    // If no connection, reset the state
    if (!locked && connected) {

        if (msg->btn_a_state == 1) {
            if (!autonomous)
                RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Autonomous Mode Enabled" << C_END);
                
            autonomous = true;
        } else if (msg->btn_b_state == 1) {
            if (autonomous)
                RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Autonomous Mode Disabled" << C_END);
            autonomous = false;
       
        } else if (msg->btn_x_state == 1) {
            if(cop_mode)
                system(("bash ~/nova/nixfiles/scripts/cop-mode.sh " + std::string(cop_mode ? "off" : "on")).c_str());
            cop_mode = !cop_mode;
        }
   
        if (msg->btn_thumb_l_state == 1) {
            if (!latest_drive_input.handbrake)
                RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Handbrake Enabled" << C_END);
            latest_drive_input.handbrake = true;
        }
            // Disable Handbrake
        else if (msg->btn_thumb_r_state == 1) {
            if (latest_drive_input.handbrake)
                RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Handbrake Disabled" << C_END);
            latest_drive_input.handbrake = false;
        }
        if (!autonomous) {
            // Change the speed multipliers
            if (msg->btn_dpad_u_state == 1)
                adjust_multiplier(multiplier_speed, true, true);
            else if (msg->btn_dpad_d_state == 1)
                adjust_multiplier(multiplier_speed, false, true);
            if (msg->btn_dpad_l_state == 1)
                adjust_multiplier(multiplier_speed, false, false);
            else if (msg->btn_dpad_r_state == 1)
                adjust_multiplier(multiplier_speed, true, false);
            if (msg->btn_y_state != 0) {
                if (latest_drive_input.mode != drive_interfaces::msg::DriveInput::TANK)
                    RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Tank Mode" << C_END);                    
                latest_drive_input.mode = drive_interfaces::msg::DriveInput::TANK;
            } else if (msg->btn_shoulder_l_state != 0) {
                if (latest_drive_input.mode != drive_interfaces::msg::DriveInput::STRAFE)
                    RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Strafe Mode" << C_END);
                latest_drive_input.mode = drive_interfaces::msg::DriveInput::STRAFE;
            } else if (msg->btn_shoulder_r_state != 0) {
                if (latest_drive_input.mode != drive_interfaces::msg::DriveInput::PIVOT)
                    RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Pivot Mode" << C_END);
                latest_drive_input.mode = drive_interfaces::msg::DriveInput::PIVOT;
            }
            trigger_speed = 1.0 - (msg->trg_r_val * (1 - MIN_TRIGGER_MULTIPLIER));
            if (latest_drive_input.mode == drive_interfaces::msg::DriveInput::STRAFE) {
                latest_drive_input.speed = -msg->ax_stick_l_x * multiplier_speed * trigger_speed;

            } else {
                latest_drive_input.speed = msg->ax_stick_l_y * multiplier_speed * trigger_speed;
            }
            
            latest_drive_input.radius = msg->ax_stick_r_x == 0 ? INFINITY : (1.0 / pow(abs(msg->ax_stick_r_x), 2)) - 1;
            latest_drive_input.direction = msg->ax_stick_r_x > 0 ? 1 : msg->ax_stick_r_x < 0 ? -1 : 0;
        }
    }
}

// Main constructor that sets up the node
DriveInputs::DriveInputs() : Node("drive_inputs")
{
    // Fill with default values on startup
    latest_drive_input.radius = INFINITY;
    latest_drive_input.mode = drive_interfaces::msg::DriveInput::TANK;
    latest_drive_input.handbrake = false;
    latest_drive_input.speed = 0.0;
    latest_drive_input.direction = 0;

    // Stores QoS options
    rclcpp::QoS qos = rclcpp::QoS(1).best_effort().deadline(DriveTimers::drive_deadline);
    rclcpp::SubscriptionOptions input_subscriber_options;

    rclcpp::SubscriptionOptions auto_subscriber_options;

    // Create the publisher with a best effort QoS policy
    drive_publisher = this->create_publisher<drive_interfaces::msg::DriveInput>("/drive/drive_inputs", qos);
    info_publisher = this->create_publisher<drive_interfaces::msg::DriveInfo>("/drive/drive_info",10);
    //Sets subscriber options before subscription is made
    input_subscriber_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {
        input_deadline_exceeded();
    };

    //Sets subscriber options before subscription is made
    auto_subscriber_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {
      auto_deadline_exceeded();
    };

    // Creates the input subscription
    gamepad_input_subscription = this->create_subscription<input_interfaces::msg::InputGamepad>(
        "/inputs/input_gamepad", qos, std::bind(&DriveInputs::input_callback, this, _1), input_subscriber_options);
    autonomous_commands_subscription = this->create_subscription<drive_interfaces::msg::DriveInput>(
        "/inputs/autonomous_commands", qos, std::bind(&DriveInputs::autonomous_callback, this, _1), auto_subscriber_options);
    // Creates a timer function that runs a function on loop every 0.05 seconds
    drive_timer = this->create_wall_timer(DriveTimers::drive_control, std::bind(&DriveInputs::publish_cmds, this));
    info_timer = this->create_wall_timer(DriveTimers::drive_info, std::bind(&DriveInputs::publish_info, this));

    RCLCPP_INFO_STREAM(this->get_logger(), C_TITLE << "Drive Controls:" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "       Left Stick Y      |  Forward/Back" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "       Right Stick X     |  Left/Right" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "      Right Trigger      |  Speed Multiplier" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "             DPAD Y      |  Speed Incr/Decr Course" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "             DPAD X      |  Speed Incr/Decr Fine" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "    Left Joy Button      |  Handbrake Enabled" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "   Right Joy Button      |  Handbrake Disabled" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "               Back      |  Lock" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "              Start      |  Unlock" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "                  A      |  Autonomous Control" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "                  B      |  Manual Control" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "                  Y      |  Tank Mode" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "       Right Bumper      |  Pivot Mode" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "       Right Bumper      |  Pivot Mode" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INPUT << "       Left Bumper       |  Strafe Mode" << C_END);
    RCLCPP_INFO_STREAM(this->get_logger(), "Gamepad Locked");
    RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Tank Mode" << C_END);
    

}

//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<DriveInputs>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
