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


// Adjustes the multiplier factor by some amount in some direction
float DriveInputs::adjust_multiplier (float& multiplier, bool increase) {

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
void DriveInputs::publish_cmds () {

    // Create the message
    auto message = core::msg::DriveInput();
    
    if (!prev_msg_received) return;

    // Set up the values if the controller is not locked
    if (!locked && connected) {
        message.speed = input_axis_y * multiplier_speed * trigger_speed;
        message.steer = input_axis_x;
    
    // Otherwise print lock message
    } else if (locked) {
        //cout << "Controller LOCKED." << endl;
        fflush(stdout);
    }
    
    // Publish the drive commands
    publisher->publish(message);

}

// Stops driving when no input received from radios for a period of time
void DriveInputs::deadline_exceeded (){

    // Clear the old inputs
    input_axis_y = 0.0;
    input_axis_x = 0.0;
    
    Print::print("No gamepad input received");
    prev_msg_received = false;
}


// Receives input from the gamepad
void DriveInputs::input_callback (const core::msg::InputGamepad::SharedPtr msg) {

    // If no connection, reset the state
    if (!msg->connected) {
        input_axis_x = 0.0;
        input_axis_y = 0.0;
        trigger_speed = 1.0;

        // Publish no connection message
        if (connected)
            Print::print ("No Gamepad Connected", C_FAIL);
    }

    // If the controller is connected
    else {

        prev_msg_received = true;

        // Publish connection message
        if (!connected)
            Print::print ("Gamepad Connected", C_SUCCESS);

        // Update the input axis
        input_axis_x = msg->ax_stick_r_x;
        input_axis_y = msg->ax_stick_l_y;

        // Get the trigger speed multiplier
        trigger_speed = 1.0 - (msg->trg_r_val * (1 - MIN_TRIGGER_MULTIPLIER));

        // Determine if the conrroller needs to be locked or not
        if (msg->btn_back_state == 1) {
            if (!locked)
                Print::print("Gamepad Locked");
            locked = true;   
        } if (msg->btn_start_state == 1) {
            if (locked)
                Print::print("Gamepad Unlocked");
            locked = false;          
        }
        

        // Prevent changing states if the controller is locked
        if (!locked) {

            // Change the speed multipliers
            if (msg->btn_dpad_u_state == 1)
                adjust_multiplier(multiplier_speed, true);
            else if (msg->btn_dpad_d_state == 1)
                adjust_multiplier(multiplier_speed, false);
        }
    }  

    // Get the connection state
    connected = msg->connected;       
}


// Main constructor that sets up the node
DriveInputs::DriveInputs() 
  : Node("drive_inputs"), count(0) {

    // Creates the publisher
    publisher = this->create_publisher<core::msg::DriveInput>("/control/drive_inputs", 10);
    
    //Sets subscriber options before subscription is made
    subscriber_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {
    deadline_exceeded();
    };

    // Creates the input subscription
    subscription = this->create_subscription<core::msg::InputGamepad>(
        "/control/input_gamepad", qos, std::bind(&DriveInputs::input_callback, this, _1), subscriber_options);

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer = this->create_wall_timer(10ms, std::bind(&DriveInputs::publish_cmds, this));

    // Output set-up messages
    Print::title("DRIVE INPUTS");
    Print::print("Valid Topics:");
    Print::print("/control/drive_inputs         [DriveInput]", 1);
    Print::print("", true);

    // Output control messages
    Print::print("Drive Controls:");
    Print::print("     Left Stick Y  |  Forward/Back", C_INPUT);
    Print::print("    Right Stick X  |  Left/Right", C_INPUT);
    Print::print("", true);
    Print::print("    Right Trigger  |  Speed Multiplier", C_INPUT);
    Print::print("           DPAD Y  |  Speed Incr/Decr", C_INPUT);
    Print::print("  Left Joy Button  |  Handbrake Enabled", C_INPUT);
    Print::print(" Right Joy Button  |  Handbrake Disabled", C_INPUT);
    Print::print("", true);
    Print::print("             Back  |  Lock", C_INPUT);
    Print::print("            Start  |  Unlock", C_INPUT);
    Print::print("                A  |  Autonomous Control", C_INPUT);
    Print::print("                B  |  Manual Control", C_INPUT);
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
