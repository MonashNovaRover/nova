/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Harrison Verrios, Josh Cherubino, Will de la Rue
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include math library
#include <cmath>

// Include the header file
#include "driver.h"
#include "print/print.h"

// Sends commands to the wheels
void Driver::send_commands (const core::msg::DriveInput::SharedPtr msg) {
    
    // Check if wheels should spin
    if (msg->speed != 0.0) {

        // Reset the stops flag
        stopped_sent = false;

        // If no steer, just spin with speed
        if (msg->steer == 0) {
            for (Wheel* wheel : wheels)
                wheel->spin(msg->speed);

            return;
        }

        // Otherwise, calculate the speed for each wheel to follow a circular path 
        
        // Find the turning radius form the 'steer' command
        // This defines a turning centre to the left or right of the rover wheelbase
        float radius = get_turning_radius(msg->steer);
        
        // Get array of wheel velocities for each wheel
        // Scale wheel velocities depending on their distance from the turning centre
        // Wheels closer to the turning centre must spin slower to maintain the correct rover angular velocity
        // The 'speed' command gives the maximum speed for any wheel
        // Disregard any correction for the angle of the wheel relative to the desired circular path
        float wheel_velocities[NUM_WHEELS];
        fill_wheel_velocities(wheel_velocities, radius, msg->speed, msg->steer);

        // Send velocities to the wheels
        for (size_t i = 0; i < NUM_WHEELS; i++) {
            wheels[i]->spin(wheel_velocities[i]);
        }
    }

    // Otherwise, if handbrake is on, send zeros
    else if (handbrake) {
        // Reset the stops flag
        stopped_sent = false;
        
        // Spin the wheels at 0 speed
        for (Wheel* wheel : wheels) {
            wheel->spin(0.0);
        }
    }

    // Otherwise, if handbrake is not on, only send stops
    else if (!stopped_sent) {
        // Stop the wheels
        for (Wheel* wheel : wheels) {
            wheel->stop();
        }

        // Set the stopped flag so it doesn't run again
        stopped_sent = true;
    }
}


// Receives drive commands
void Driver::drive_callback (const core::msg::DriveInput::SharedPtr msg) {

    // If manual driving state, call the commands
    if (!is_autonomous)
        send_commands(msg);    
}


// Receives autonomous commands
void Driver::auto_callback (const core::msg::DriveInput::SharedPtr msg) {

    // If autonomous driving state, call the commands
    if (is_autonomous)
        send_commands(msg); 
}


// Receives input from the gamepad
void Driver::input_callback (const core::msg::InputGamepad::SharedPtr msg) {

    // Enable handbraking based on the thumb buttons
    if (msg->connected && msg->btn_thumb_l_state == 1) {
        if (!handbrake) Print::print("Handbrake Enabled", C_MODE);
        handbrake = true;
    }
    
    // Disable Handbrake
    else if (msg->connected && msg->btn_thumb_r_state == 1) {
        if (handbrake) Print::print("Handbrake Disabled", C_MODE);
        handbrake = false;
    }

    // Enable  autonomous
    if (msg->connected && msg->btn_a_state == 1) {
        if (!is_autonomous) Print::print("Mode: Autonomous", C_MODE);
        is_autonomous = true;
    }

    // Disable autonomous
    else if (msg->connected && msg->btn_b_state == 1) {
        if (is_autonomous) Print::print("Mode: Manual", C_MODE);
        is_autonomous = false;
    }
}


// Gets the turning radius of the rover
float Driver::get_turning_radius (float steer) {
    // Exclude this case. If steer is 0, handle separately in calling code
    if (steer == 0) return NAN;

    // Map a steer mgnitude of 1 to a radius of 0 (turning on the spot)
    // Map a steer magnitude near 0 to a radius near infinity
    // Maintain the sign of steer in the sign of radius
    return (1.0 / steer) - ((steer < 0.0) ? -1.0 : 1.0);
}


// Fill array with velocities for each wheel, with directions and magnitude depending on the turning radius
void Driver::fill_wheel_velocities(float wheel_velocities[NUM_WHEELS], float radius, float speed, float steer) {
    
    // Calculate distances from the wheelbase centre to each wheel, and the maximum distance
    float distances[NUM_WHEELS];
    float max_distance = 0;
    for (size_t i = 0; i < NUM_WHEELS; i++){
        Vector2 position = get_wheel_position(wheels[i]->get_id());
        distances[i] = get_wheel_distance(position, radius);
        if (distances[i] > max_distance) max_distance = distances[i];
    }

    // Fill wheel velocities, scaling each by its distance to the wheelbase centre
    // Approximating that the wheels drive tangent to the turning circle, the scaling ensures
    // each wheel achieves the same angular velocity about the turning center the rover
    for (size_t i = 0; i < NUM_WHEELS; i++){
        wheel_velocities[i] = speed * distances[i] / max_distance;
    }
    
    // Modify wheel directions if the turning centre is under the rover wheelbase
    // Ignore the edge case where the turning centre is exactly below the centre of a wheel.
    // Does not affect the behaviour in practice
    float wheel_x = CHASSIS_SEPARATION / 2.0;
    if (abs(radius) < wheel_x) {
        // If the turning centre is...
        if (radius > -wheel_x && radius <= 0 && steer < 0) {
            // Under the left half of the chassis, reverse the left wheels
            // Also include cases where we are pivoting left
            wheel_velocities[0] *= -1;
            wheel_velocities[1] *= -1;
            wheel_velocities[2] *= -1;
        }
        else if (radius >= 0 && radius < wheel_x && steer > 0) {
            // Under the right half of the chassis, reverse the right wheels
            // Also include cases where we are pivoting right
            wheel_velocities[4] *= -1;
            wheel_velocities[5] *= -1;
            wheel_velocities[6] *= -1;
        }
    }
}


// Gets the position of the wheel relative to the wheelbase centre
// Wheels are numbered 1 to 6 going from front to back on the left side, then front to back on the right side
Vector2 Driver::get_wheel_position (int id) {

    // Determine the y position
    float y = 0;
    if (id == 1 || id == 4) y = WHEEL_SEPARATION;
    else if (id == 3 || id == 6) y = -WHEEL_SEPARATION;

    // Determine the x position
    float wheel_x = (CHASSIS_SEPARATION / 2.0) * ((id <= 3) ? -1.0 : 1.0);

    // Return the vector struct
    return Vector2(wheel_x, y);
}


// Determine the distance between the wheel and the turning centre
float Driver::get_wheel_distance (Vector2 pos, float radius) {

    // Calculate the x component
    float x = radius - pos.x;

    // Find pythagorus distance
    return sqrt(pow(x, 2) + pow(pos.y, 2));
}


// Publishes whether or not we are in autonomous mode
void Driver::pub_auto_mode (){

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
    for (size_t i = 0; i < NUM_WHEELS; i++) {
        bool left = i < NUM_WHEELS / 2;
        wheels[i] = new Wheel (i + 1, left);
    }
    
    rclcpp::QoS qos = rclcpp::QoS(1).best_effort().deadline(200ms);

    rclcpp::SubscriptionOptions subscriber_options;
	
    //Sets subscriber options before subscription is made
	subscriber_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {inputs_deadline_exceeded();};

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
        mode_timer_period, std::bind(&Driver::pub_auto_mode, this)
    );
    mode_pub = this->create_publisher<std_msgs::msg::Bool>(
        "/autonomous/mode", 10
    );
}

// deadline callback for when the drive inputs publisher misses its deadline
void Driver::inputs_deadline_exceeded(){
	RCLCPP_WARN(this->get_logger(), "Drive inputs subscriber deadline missed");
    for (Wheel* wheel : wheels) wheel->stop();
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

