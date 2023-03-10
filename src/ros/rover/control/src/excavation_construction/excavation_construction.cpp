/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Manika Goyal
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "excavation_construction.h"
#include "print/print.h"
#include "config/rosconfig.h"

using std::placeholders::_1;

float ExcavationConstruction::get_can_commands () {
        Frame scraper_arm;
        Frame scraper_scoop;
        Frame tile_placer;

        scraper_arm.data.push_back(0);
        scraper_arm.data.push_back(scraper_arm_direction);        
        scraper_arm.data.push_back((uint8_t) (scraper_arm_velocity >> 8));
        scraper_arm.data.push_back((uint8_t) (scraper_arm_velocity & 0xFF));

        scraper_scoop.data.push_back(0);
        scraper_scoop.data.push_back(scraper_scoop_direction);        
        scraper_scoop.data.push_back((uint8_t) (scraper_scoop_velocity >> 8));
        scraper_scoop.data.push_back((uint8_t) (scraper_scoop_velocity & 0xFF));

        tile_placer.data.push_back(0);
        tile_placer.data.push_back(tile_placer_direction);        
        tile_placer.data.push_back((uint8_t) (tile_placer_velocity >> 8));
        tile_placer.data.push_back((uint8_t) (tile_placer_velocity & 0xFF));

        return (scraper_arm.data, scraper_scoop.data, tile_placer.data);

}

void ExcavationConstruction::joystick_l_callback (const core::msg::InputJoystick::SharedPtr msg) {
    
    joystick_l = *msg;

    // Joysticks lock
    if (joystick_l.btn_bottom_l2_state == 1) {
        if (!joystick_locked)
            Print::print("Joysticks locked");
        joystick_locked = true;
    }
    if (joystick_l.btn_bottom_l5_state == 1){
        if (joystick_locked)
            Print::print("Joysticks Unlocked");
        joystick_locked = false;
    }

    // Update the inputs
    scraper_arm_velocity = abs(joystick_l.ax_stick_x);
    scraper_arm_direction = (joystick_l.ax_stick_x >= 0) ? 1:2
}

void ExcavationConstruction::joystick_r_callback (const core::msg::InputJoystick::SharedPtr msg) {
    
    joystick_r = *msg;

    // Tile Placer Activated
    if (joystick_r.btn_thumb_l_state == 1 || joystick_r.btn_thumb_d_state == 1) {
        tile_placer_activated = true;
        // Update the inputs
        scraper_scoop_velocity = 0;
        tile_placer_velocity = abs( 255 * joystick_l.ax_stick_x );
        scraper_arm_velocity = 0;
        tile_placer_direction = (joystick_l.ax_stick_x >= 0) ? 5:6
    }
    else {
        tile_placer_activated = false;
        // Update the inputs
        scraper_scoop_velocity = abs( 255 * joystick_r.ax_stick_x);
        tile_placer_velocity = 0;
        scraper_scoop_direction = (joystick_r.ax_stick_x >= 0) ? 3:4
    }  
}

// Resets joystick internal state
void ExcavationConstruction::joystick_deadline_callback()
{
    RCLCPP_WARN(this->get_logger(), "Joystick subscriber deadline missed");
    joystick_l = core::msg::InputJoystick();
    joystick_r = core::msg::InputJoystick();
}


// Publishes the drive commands from the speed and steer
void ExcavationConstruction::publish_cmds () {

    // Create the message
    auto message = core::msg::ExcavationConstruction();
    
    // Set up the values if the controller is not locked
    if (!locked && connected) {
        message.tile_placer_velocity = tile_placer_velocity // sends value from -1 to 1. TODO: Scale it
        message.scraper_arm_velocity = scraper_arm_velocity // sends value from -1 to 1. TODO: Scale it
        message.scraper_scoop_velocity = scraper_scoop_velocity // sends value from -1 to 1. TODO: Scale it
        
    // Otherwise print lock message
    } else if (locked) {
        //cout << "Joystick LOCKED." << endl;
        fflush(stdout);
    }
    
    // Publish the drive commands
    publisher->publish(message);

}

// Main constructor that sets up the node
ExcavationConstruction::ExcavationConstruction() : Node("excavation_construction");
{

    // Stores QoS options
    rclcpp::QoS qos = rclcpp::QoS(1).best_effort().deadline(ROSTimers::);
    rclcpp::SubscriptionOptions subscriber_options;

    this->declare_parameter("canbus", "can0");
    this->get_parameter("canbus").get_parameter_value().get<std::string>()


    // Create the publisher with a best effort QoS policy
    publisher = this->create_publisher<core::msg::ExcavationConstruction>("/control/excavation_construction", qos);
    
    //Sets subscriber options before subscription is made
    subscriber_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void {
        deadline_exceeded();
    };

   // Creates the input subscription for the left joystick (with QoS options)
    joystick_l_sub = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_l",
        joystick_qos,
        std::bind(&ExcavationConstruction::joystick_l_callback, this, _1),
        joystick_options
    );

    // Creates the input subscription for the right joystick (with QoS options)
    joystick_r_sub = this->create_subscription<core::msg::InputJoystick>(
        "/control/input_joystick_r",
        joystick_qos,
        std::bind(&ExcavationConstruction::joystick_r_callback, this, _1),
        joystick_options
    );

    // Creates a timer function that runs a function on loop every 0.05 seconds
    timer = this->create_wall_timer(ROSTimers::drive_control, std::bind(&DriveInputs::publish_cmds, this));
    
    vector excavationConstructionCommands = get_can_commands();

    if (excavationConstructionCommands)

        Frame scraperScoopFrame = new_frame(0x0A0, {});
        printf("Sending: %s...\n",scraperScoopFrame.to_string().c_str());
        bus->send(scraperScoopFrame);

        Frame scraperArmFrame = new_frame(0x0A0, {});
        printf("Sending: %s...\n",scraperArmFrame.to_string().c_str());
        bus->send(scraperArmFrame);

        Frame tilePlacerFrame = new_frame(0x0A0, {});
        printf("Sending: %s...\n",tilePlacerFrame.to_string().c_str());
        bus->send(tilePlacerFrame);

    // Output set-up messages
    Print::title("EXCAVATION CONSTRUCTION");
    Print::print("Subscribed Topics:");
    Print::print("/control/input_joystick_l            [core/InputJoystick]", 1);
    Print::print("/control/input_joystick_r            [core/InputJoystick]", 1);
    Print::print("Published Topics:");
    Print::print("/control/excavation_construction         [ExcavationConstruction]", 1);
    Print::print("", true);
}


//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<ExcavationConstruction>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
