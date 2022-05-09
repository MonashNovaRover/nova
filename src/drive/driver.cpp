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

        // Otherwise, calculate the speed

        // Store a new array of contants
        float distances[NUM_WHEELS];
        float tangents[NUM_WHEELS];

        // Stores some of the maximum values
        float locas = get_locas_distance(msg->steer);
        float max_distance = 0;
        float max_tangent = 0;

        // Determine the distance and tangent ratios
        for (int i = 0; i < NUM_WHEELS; i++) {

            Vector2 position = get_wheel_position(wheels[i]->get_id());

            // Calculate the max distance to the wheels and store them
            float dist = get_wheel_distance(position, locas);
            distances[i] = dist;
            if (dist > max_distance) max_distance = dist;          

            // Calculate the tangent ratios and store them
            float tangent = get_tangent_scale(position, locas);
            tangents[i] = tangent;
            if (tangent > max_tangent) max_tangent = tangent;
        }
    
        // Loop through each wheel to calculate speeds
        for (int i = 0; i < NUM_WHEELS; i++) {
            // Calculate the velocity of wheel
            float vel = msg->speed * distances[i] / max_distance;
            
            // If using tangent scaling, adjust for wheel speeds
            if (USE_TANGENT_SCALING) vel *= tangents[i] / max_tangent;
            
            // Checks if the turning circle is within the chassis area
            if (abs(locas) < CHASSIS_SEPARATION / 2.0) {
                // Check if going left and left wheels
                if (locas < 0 && i <= 2) vel *= -1.0;
                
                // Check if going right and right wheels
                else if (locas > 0 && i > 2) vel *= -1.0;
                
                // Check if turning on spot
				else if (locas == 0) {
				    // If wanting to turn left and left wheels
					if (msg->steer < 0 && i <= 2) vel *= -1.0;
					
					// If wanting to turn right and right wheels
					else if (msg->steer > 0 && i > 2) vel *= -1.0;
				}
            }

            // Send the velocities to the wheels
            wheels[i]->spin(vel);
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


// Gets the distance to the locas of the turning circle
float Driver::get_locas_distance (float steer) {
    if (steer == 0) return 0;

    // Return the calculation
    return (1.0 / steer) - ((steer < 0.0) ? -1.0 : 1.0);
}


// Gets the position of the wheel relative to the CoM
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


// Determines the distance between the wheel and the focus
float Driver::get_wheel_distance (Vector2 pos, float locas) {

    // Calculate the x component
    float x = locas - pos.x;

    // Find pythagorus distance
    return sqrt(pow(x, 2) + pow(pos.y, 2));
}


// Determines the tangent scale of the wheel
float Driver::get_tangent_scale (Vector2 pos, float locas) {

    // Calculate the x component
    float x = locas - pos.x;

    // Find pythagorus distance
    return sqrt(1.0 + pow(pos.y / x, 2));
}


// Main constructor that sets up the node
Driver::Driver() : Node("driver")
{
    // Output set-up messages
    Print::title("DRIVER");
    Print::print("", true);

    // Initialise the wheels in the correct direction
    for (int i = 0; i < NUM_WHEELS; i++) {
        bool left = i < NUM_WHEELS / 2;
        wheels[i] = new Wheel (i + 1, left);
    }

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
        "/control/input_gamepad", qos, std::bind(&Driver::input_callback, this, _1));
}

void Driver::inputs_deadline_exceeded(){
	RCLCPP_WARN(this->get_logger(), "Inputs subscriber deadline missed");
    // Spin the wheels at 0 speed
    for (Wheel* wheel : wheels) {
	    wheel->spin(0.0);
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

