/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino, Will de la Rue, Jory Braun, Tristan Clark, Abigail Lithwick, Taaj Street
~~~~~~~~~~~~~~~~~~~~~~~~~~~~c~~~~~~~~~~~~~~~~~~~~~~
*/

// Include math library
#include <cmath>

// Include the header file
#include "driver.h"
#include "print/print.h"
#include "config/rosconfig.h"

// Use the standard namespaces for subscribers
using std::placeholders::_1;

// Sends commands to the wheels
void Driver::send_commands(const core::msg::DriveInput::SharedPtr msg)
{

        if (!msg->strafe_mode)
        {
            // Find the turning radius form the 'steer' command
            // This defines a turning centre to the left or right of the rover wheelbase
            float radius = get_turning_radius(msg->steer);

            // Scale wheel velocities depending on their distance from the turning centre
            // Wheels closer to the turning centre must spin slower to maintain the correct rover angular velocity
            // The 'speed' command gives the maximum speed for any wheel
            // Disregard any correction for the angle of the wheel relative to the desired circular path
            // fill_wheel_velocities(wheel_velocities, radius, msg->speed, msg->steer);

            fill_wheel_angles_radial(radius);
            fill_wheel_velocities_radial(msg->speed, msg->steer);
        }
        else
        {
            /*
            float turning_angle = msg->steer;

            fill_wheel_angles_strafe(wheels, msg->steer);
            fill_wheel_velocities_strafe(wheels, msg->speed, msg->steer);
            */
        }


    auto message = core::msg::PivotWheelData();

    // Send velocities to the wheels
    for (size_t i = 0; i < NUM_WHEELS; i++)
    {
        // wheels[i]->drive(wheel_velocities[i]);
        message.angles[i] = wheels[i]->angle;
        message.velocities[i] = wheels[i]->velocity;
    }
    RCLCPP_INFO(this->get_logger(), "Publishing!");
    pivot_wheel_pub->publish(message);
}

// Receives drive commands
void Driver::drive_callback(const core::msg::DriveInput::SharedPtr msg)
{

    // If manual driving state, call the commands
    if (!is_autonomous)
        send_commands(msg);
}

// Receives autonomous commands
void Driver::auto_callback(const core::msg::DriveInput::SharedPtr msg)
{

    // If autonomous driving state, call the commands
    if (is_autonomous)
        send_commands(msg);
}

// Receives input from the gamepad
void Driver::input_callback(const core::msg::InputGamepad::SharedPtr msg)
{

    // Enable handbraking based on the thumb buttons
    if (msg->connected && msg->btn_thumb_l_state == 1)
    {
        if (!handbrake)
            Print::print("Handbrake Enabled", C_MODE);
        handbrake = true;
        for (Wheel *wheel : wheels)
        {
            wheel->cmdWheel->set_CMD_stop_mode(PID);
            wheel->cmdPivot->set_CMD_stop_mode(PID);
        }
    }

    // Disable Handbrake
    else if (msg->connected && msg->btn_thumb_r_state == 1)
    {
        if (handbrake)
            Print::print("Handbrake Disabled", C_MODE);
        handbrake = false;
        for (Wheel *wheel : wheels)
        {
            wheel->cmdWheel->set_CMD_stop_mode(STOP);
            wheel->cmdPivot->set_CMD_stop_mode(STOP);
        }
    }

    // Enable  autonomous
    if (msg->connected && msg->btn_a_state == 1)
    {
        if (!is_autonomous)
            Print::print("Mode: Autonomous", C_MODE);
        is_autonomous = true;
    }

    // Disable autonomous
    else if (msg->connected && msg->btn_b_state == 1)
    {
        if (is_autonomous)
            Print::print("Mode: Manual", C_MODE);
        is_autonomous = false;
    }
}

// Gets the turning radius of the rover
float Driver::get_turning_radius(float steer)
{
    // Exclude this case. If steer is 0, handle separately in calling code
    if (steer == 0)
        return INFINITY;

    // SHOULD STEER BE -1 to 1 ??

    return (MAX_RADIUS - CHASSIS_WIDTH / 2) * abs(steer) + MAX_RADIUS;
}

void Driver::fill_wheel_angles_radial(float radius)
{
    // gradient of line https://www.desmos.com/calculator/cb3xa9r5ai

    wheels[0]->angle = atan(-(2 * radius - CHASSIS_WIDTH) / (CHASSIS_LENGTH));

    wheels[1]->angle = atan(-(2 * radius + CHASSIS_WIDTH) / (CHASSIS_LENGTH));

    wheels[2]->angle = atan(-(2 * radius - CHASSIS_WIDTH) / (-CHASSIS_LENGTH));

    wheels[3]->angle = atan(-(2 * radius + CHASSIS_WIDTH) / (-CHASSIS_LENGTH));
}

// Fill array with velocities for each wheel, with directions and magnitude depending on the turning radius
void Driver::fill_wheel_velocities_radial(float speed, float steer)
{
    // determines whether the wheels are turning left or right
    int direction = (steer < 0) ? -1 : 1;

    for (size_t i = 0; i < NUM_WHEELS; i++)
    {
        if (i == 0 || i == 2)
        {
            wheels[i]->velocity = (pow(CHASSIS_LENGTH, 2.0) / 4) + pow(speed + direction * (CHASSIS_WIDTH / 2), 2.0);
        }
        else if (i == 1 || i == 3)
        {
            wheels[i]->velocity = (pow(CHASSIS_LENGTH, 2.0) / 4) + pow(speed - direction * (CHASSIS_WIDTH / 2), 2.0);
        }
    }
}

// Publishes whether or not we are in autonomous mode
void Driver::pub_auto_mode()
{

    // Construct a message from our current is_autonomous boolean
    std_msgs::msg::Bool msg;
    msg.data = is_autonomous;

    // publish the message
    mode_pub->publish(msg);
}

// Main constructor that sets up the node
Driver::Driver() : Node("driver")
{
    // Output set-up messages
    Print::title("DRIVER");
    Print::print("", true);

    // Initialise the wheels in the correct direction
    for (size_t i = 0; i < NUM_WHEELS; i++)
    {
        bool left = i < NUM_WHEELS / 2;
        CMD *cmdWheel = new CMD(0, i + 1, PID, STOP, left);
        CMD *cmdPivot = new CMD(0, i + NUM_WHEELS + 1, PID, STOP, left);
        wheels[i] = new Wheel(i, cmdWheel, cmdPivot);
    }
    
    rclcpp::QoS qos = rclcpp::QoS(1).best_effort().deadline(ROSTimers::drive_deadline);

    rclcpp::SubscriptionOptions subscriber_options;

    // Sets subscriber options before subscription is made
    subscriber_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void
    { inputs_deadline_exceeded(); };

    // Creates the commands subscription (manual)
    subscription_cmds_man = this->create_subscription<core::msg::DriveInput>(
        "/control/drive_inputs", qos, std::bind(&Driver::drive_callback, this, _1), subscriber_options);

    // Creates the commands subscription (autonomous)
    subscription_cmds_auto = this->create_subscription<core::msg::DriveInput>(
        "/autonomous/drive_inputs", 10, std::bind(&Driver::auto_callback, this, _1));

    // Creates the input subscription
    subscription_inputs = this->create_subscription<core::msg::InputGamepad>(
        "/control/input_gamepad", qos, std::bind(&Driver::input_callback, this, _1), subscriber_options);

    // Creates auto mode timer and associated publisher
    mode_timer = this->create_wall_timer(
        ROSTimers::auto_mode, std::bind(&Driver::pub_auto_mode, this));
    mode_pub = this->create_publisher<std_msgs::msg::Bool>(
        "/autonomous/mode", 10);

    pivot_wheel_pub = this->create_publisher<core::msg::PivotWheelData>("/control/pivot_wheel_data", 10);
}

// deadline callback for when the drive inputs publisher misses its deadline
void Driver::inputs_deadline_exceeded()
{
    RCLCPP_WARN(this->get_logger(), "Drive inputs subscriber deadline missed");
    for (Wheel *wheel : wheels)
    {
        wheel->cmdPivot->stop();
        wheel->cmdWheel->stop();
    }
}

//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Subscriber class
    rclcpp::spin(std::make_shared<Driver>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
