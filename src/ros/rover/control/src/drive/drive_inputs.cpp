/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Liam Whittle
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "drive_inputs.h"
#include "print/print.h"
#include "config/rosconfig.h"

using std::placeholders::_1;

#include <math.h>

// Adjustes the multiplier factor by some amount in some direction
float DriveInputs::adjust_multiplier(float &multiplier, bool increase)
{

    // Adjust the multiplier
    multiplier += (increase) ? DELTA_MULTIPLIER : -DELTA_MULTIPLIER;

    // Check for minimum and maximums
    if (multiplier > MAX_MULTIPLIER)
        multiplier = MAX_MULTIPLIER;
    else if (multiplier <= MIN_MULTIPLIER)
        multiplier = MIN_MULTIPLIER;

    // Return the new multiplier
    return multiplier;
}

// Publishes the drive commands from the speed and steer
void DriveInputs::publish_cmds()
{
    if (prev_msg_received) {
        auto message = core::msg::DriveInput();
        message.speed = speed;
        message.radius = radius;
        message.direction = direction;
        message.mode = mode;
        message.handbrake = handbrake;

        // Publish the drive commands
        publisher->publish(message);

        // Clear the msg_received flag
        prev_msg_received = false;
    }
}

// Stops driving when no input received from radios for a period of time
void DriveInputs::input_deadline_exceeded()
{
    // Clear the old inputs
    speed = 0.0;
    Print::print("No gamepad input received");
    RCLCPP_WARN(this->get_logger(), "Input gamepad subscriber deadline missed");
    prev_msg_received = false;
}

void DriveInputs::auto_deadline_exceeded() {
    speed = 0.0;
    prev_msg_received = false;
    Print::print("No autonomous input received");
    RCLCPP_WARN(this->get_logger(), "Autonomous input subscriber deadline missed");
}

void DriveInputs::autonomous_callback(const  core::msg::DriveInput::SharedPtr msg) {
    if (autonomous){
        mode = msg->mode;
        speed = msg->speed;
        radius = msg->radius;
        direction = msg->direction;
    }
}

// Receives input from the gamepad
void DriveInputs::input_callback(const core::msg::InputGamepad::SharedPtr msg)
{
    prev_msg_received = true;
    if (!msg->connected)
    {
        radius = INFINITY;
        direction = 0;
        speed = 0.0;
        // Publish no connection message
        if (connected)
            Print::print("No Gamepad Connected", C_FAIL);
    } else {
        if (!connected)
            Print::print("Gamepad Connected", C_SUCCESS);
    }
    connected = msg->connected;

    if (msg->btn_back_state == 1)
    {
        if (!locked)
            Print::print("Gamepad Locked");
        locked = true;
    }
    if (msg->btn_start_state == 1)
    {
        if (locked)
            Print::print("Gamepad Unlocked");
        locked = false;
    }
    // Determine if the conrroller needs to be locked or not
    // If no connection, reset the state
    if (!locked && connected) {

        if (msg->btn_a_state == 1) {
            if (!autonomous)
                Print::print("Autonomous Mode Enabled", C_MODE);
            autonomous = true;
        } else if (msg->btn_b_state == 1) {
            if (autonomous)
                Print::print("Autonomous Mode Disabled", C_MODE);
            autonomous = false;
        }

        if (msg->btn_thumb_l_state == 1) {
            if (!handbrake)
                Print::print("Handbrake Enabled", C_MODE);
            handbrake = true;
        }
            // Disable Handbrake
        else if (msg->btn_thumb_r_state == 1) {
            if (handbrake)
                Print::print("Handbrake Disabled", C_MODE);
            handbrake = false;
        }
        if (!autonomous) {
            // Change the speed multipliers
            if (msg->btn_dpad_u_state == 1)
                adjust_multiplier(multiplier_speed, true);
            else if (msg->btn_dpad_d_state == 1)
                adjust_multiplier(multiplier_speed, false);

            if (msg->btn_y_state != 0) {
                if (mode != core::msg::DriveInput::TANK)
                    Print::print("Tank Mode", C_MODE);
                mode = core::msg::DriveInput::TANK;
            } else if (msg->btn_shoulder_l_state != 0) {
                if (mode != core::msg::DriveInput::STRAFE)
                    Print::print("Strafe Mode", C_MODE);
                mode = core::msg::DriveInput::STRAFE;
            } else if (msg->btn_shoulder_r_state != 0) {
                if (mode != core::msg::DriveInput::PIVOT)
                    Print::print("Pivot Mode", C_MODE);
                mode = core::msg::DriveInput::PIVOT;
            }
            trigger_speed = 1.0 - (msg->trg_r_val * (1 - MIN_TRIGGER_MULTIPLIER));
            if (mode == core::msg::DriveInput::STRAFE) {
                speed = -msg->ax_stick_l_x * multiplier_speed * trigger_speed;

            } else {
                speed = msg->ax_stick_l_y * multiplier_speed * trigger_speed;
            }

            radius = abs(msg->ax_stick_r_x == 0 ? INFINITY : (1.0 / msg->ax_stick_r_x) -
                                                                  (msg->ax_stick_r_x > 0 ? 1 : -1));
            direction = msg->ax_stick_r_x > 0 ? 1 : msg->ax_stick_r_x < 0 ? -1 : 0;

            // Otherwise print lock message
        } else if (locked) {
            // cout << "Controller LOCKED." << endl;
            fflush(stdout);
        }
    }
}

// Main constructor that sets up the node
DriveInputs::DriveInputs() : Node("drive_inputs")
{

    // Stores QoS options
    rclcpp::QoS qos = rclcpp::QoS(1).best_effort().deadline(ROSTimers::drive_deadline);
    rclcpp::SubscriptionOptions input_subscriber_options;

    rclcpp::SubscriptionOptions auto_subscriber_options;

    // Create the publisher with a best effort QoS policy
    publisher = this->create_publisher<core::msg::DriveInput>("/control/drive_inputs", qos);

    //Sets subscriber options before subscription is made
    input_subscriber_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {
        input_deadline_exceeded();
    };

    //Sets subscriber options before subscription is made
    auto_subscriber_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {
      auto_deadline_exceeded();
    };

    // Creates the input subscription
    gamepad_input_subscription = this->create_subscription<core::msg::InputGamepad>(
        "/control/input_gamepad", qos, std::bind(&DriveInputs::input_callback, this, _1), input_subscriber_options);
    autonomous_commands_subscription = this->create_subscription<core::msg::DriveInput>(
        "/control/autonomous_commands", qos, std::bind(&DriveInputs::autonomous_callback, this, _1), auto_subscriber_options);
    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer = this->create_wall_timer(ROSTimers::drive_control, std::bind(&DriveInputs::publish_cmds, this));

    // Output set-up messages
    Print::title("DRIVE INPUTS");
    Print::print("Valid Topics:");
    Print::print("/control/drive_inputs         [DriveInput]", 1);
    Print::print("", true);

    // Output control messages
    Print::print("Drive Controls:");
    Print::print("       Left Stick Y      |  Forward/Back", C_INPUT);
    Print::print("      Right Stick X      |  Left/Right", C_INPUT);
    Print::print("", true);
    Print::print("Left + Right Bumper      |  Strafe Mode", C_INPUT);
    Print::print("      Right Trigger      |  Speed Multiplier", C_INPUT);
    Print::print("             DPAD Y      |  Speed Incr/Decr", C_INPUT);
    Print::print("    Left Joy Button      |  Handbrake Enabled", C_INPUT);
    Print::print("   Right Joy Button      |  Handbrake Disabled", C_INPUT);
    Print::print("", true);
    Print::print("               Back      |  Lock", C_INPUT);
    Print::print("              Start      |  Unlock", C_INPUT);
    Print::print("                  A      |  Autonomous Control", C_INPUT);
    Print::print("                  B      |  Manual Control", C_INPUT);
    Print::print("", true);
    Print::print("Gamepad Locked");
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
