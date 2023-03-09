/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Taaj Street, Harrison Verrios, Josh Cherubino, Will de la Rue, Jory Braun, Tristan Clark, Abigail Lithwick
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

// Include the header file
#include "driver.h"
#include "print/print.h"
#include "config/rosconfig.h"

// Use the standard namespaces for subscribers
using std::placeholders::_1;
using namespace std;

// Sends commands to the wheels
void Driver::send_commands(const core::msg::DriveInput::SharedPtr msg)
{

    core::msg::PivotWheelData data_msg;
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

        fill_wheel_angles_radial(radius, msg->steer);
        fill_wheel_velocities_radial(msg->speed, radius);
        data_msg.radius = radius;
    }

    // Send velocities to the wheels
    for (size_t i = 0; i < NUM_WHEELS; i++)
    {
        PivotModule *pivot = pivots[i];
        pivot->cmdWheel->drive(pivot->velocity);
        pivot->cmdPivot->drive(pivot->angle);
        data_msg.angles[i] = pivot->angle;
        data_msg.velocities[i] = pivot->velocity;
    }
    data_msg.steer = steer;
    pivot_wheel_pub->publish(data_msg);
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
        for (PivotModule *pivot : pivots)
        {
            pivot->cmdWheel->set_stop_mode(DRIVE_VELOCITY);
        }
    }

    // Disable Handbrake
    else if (msg->connected && msg->btn_thumb_r_state == 1)
    {
        if (handbrake)
            Print::print("Handbrake Disabled", C_MODE);
        handbrake = false;
        for (PivotModule *pivot : pivots)
        {
            pivot->cmdWheel->set_stop_mode(STOP);
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

    float radius = steer == 0 ? INFINITY : (1.0 / steer) - (steer > 0 ? 1 : -1);

    cout << "radius: " << radius << endl;
    cout << "steer: " << steer << endl;

    double sign = steer == 0 ? prev_sign : (steer > 0 ? 1.0 : -1.0);
    prev_sign = sign;
    float max_change = -INFINITY;
    int index = 0;
    //Find the wheel that has to turn the most
    for (size_t i = 0; i < NUM_WHEELS; i++)
    {
        cout << "pivot " << i + 1 << " current angle: " << pivots[i]->angle << endl;
        float target = calc_wheel_angle(radius, i, sign);
        cout << "pivot " << i + 1 << " target angle: " << target << endl;
        // Get the maximum change in angle
        if (abs(target - pivots[i]->angle) > max_change) {
            max_change = abs(target - pivots[i]->angle);
            index = i;
        }
    }
    cout << "index: " << index << endl;
    float target = calc_wheel_angle(radius, index, sign);
    int direction = pivots[index]->angle == target ? 0 : (pivots[index]->angle < target ? 1 : -1);
    cout << "direction: " << direction << endl;
    // Set the targets to the minimum of the actual target and the target + the angle of the pivot modules
    cout << "target: " << target << endl;
    float theta = pivots[index]->angle + direction * d_theta;
    cout << "theta: " << theta << endl;
    if (abs(theta - target) < d_theta) theta = target;
    cout << "theta: " << theta << endl;
    cout << "actual radius: " << radius_from_angle(theta, index) << endl;
    cout << "----------------------------" << endl;
    return radius_from_angle(theta, index);
}

float Driver::calc_wheel_angle(float radius, int wheel, int sign)
{
    float angle;
    // gradient of line https://www.desmos.com/calculator/opj8exj9gp
    switch(wheel){
        case 0:
            angle = (radius == INFINITY ? 0 : atan((2*radius + CHASSIS_WIDTH)/CHASSIS_LENGTH) - sign * M_PI_2) - angle_offset;
            break;
        case 1:
            angle = (radius == INFINITY ? 0 : atan((2*radius + CHASSIS_WIDTH)/-CHASSIS_LENGTH) + sign * M_PI_2) + angle_offset;
            break;
        case 2:
            angle = (radius == INFINITY ? 0 : atan((2*radius - CHASSIS_WIDTH)/-CHASSIS_LENGTH) + sign * M_PI_2) - angle_offset;
            break;
        case 3:
            angle = (radius == INFINITY ? 0 : atan((2*radius - CHASSIS_WIDTH)/CHASSIS_LENGTH) - sign * M_PI_2) + angle_offset;
            break;
    }

    return angle;
}

float Driver::radius_from_angle(double angle, int wheel) {


    float radius;
    int sign;
    switch(wheel){
        case 0:
            sign = angle < -angle_offset ? 1 : -1;
            radius = (angle == -angle_offset ? INFINITY : (tan(angle + angle_offset + sign*M_PI_2) * CHASSIS_LENGTH/2 - CHASSIS_WIDTH/2));
            break;
        case 1:
            sign = angle < angle_offset ? 1 : -1;
            radius = (angle == angle_offset ? INFINITY : (tan(angle - angle_offset - sign*M_PI_2) * -CHASSIS_LENGTH/2 - CHASSIS_WIDTH/2));
            break;
        case 2:
            sign = angle < -angle_offset ? 1 : -1;
            radius = (angle == -angle_offset ? INFINITY : (tan(angle + angle_offset - sign*M_PI_2) * -CHASSIS_LENGTH/2 + CHASSIS_WIDTH/2));
            break;
        case 3:
            sign = angle < angle_offset ? 1 : -1;
            radius = (angle == angle_offset ? INFINITY : (tan(angle - angle_offset + sign*M_PI_2) * CHASSIS_LENGTH/2 + CHASSIS_WIDTH/2));
    }
    return radius;
}


void Driver::fill_wheel_angles_radial(float radius, float steer)
{
    cout << "Fill wheel angles" << endl;
    cout << "radius: " << radius << endl;
    cout << "steer: " << steer << endl;
    // Fill pivot module angles
    for(int i = 0; i < NUM_WHEELS; i++)
    {
        pivots[i]->angle = calc_wheel_angle(radius, i, steer > 0 ? 1.0 : -1.0);
        cout << "pivot " << i + 1 << " angle: " << pivots[i]->angle << endl;
    }
    cout << "----------------------------" << endl;
}

// Fill array with velocities for each wheel, with directions and magnitude depending on the turning radius
void Driver::fill_wheel_velocities_radial(float speed, float radius)
{
    float left_ratio = !radius ? 1 : sqrt(pow(CHASSIS_LENGTH, 2.0)/4 + pow(radius + (CHASSIS_WIDTH / 2), 2.0))/abs(radius);
    float right_ratio = !radius ? 1 : sqrt(pow(CHASSIS_LENGTH, 2.0)/4 + pow(radius - (CHASSIS_WIDTH / 2), 2.0))/abs(radius);
    float max = 0;
    for (size_t i = 0; i < NUM_WHEELS; i++)
    {
        if (i < 2)
        {
            pivots[i]->velocity = radius == INFINITY ? speed : speed*left_ratio;
        }
        else
        {
            pivots[i]->velocity =  radius == INFINITY ? speed : speed*right_ratio;
        }

        if (abs(pivots[i]->velocity) > max) max = abs(pivots[i]->velocity);

    }

    if (max > 0.35) {
        for (size_t i = 0; i < NUM_WHEELS; i++)
        {
            pivots[i]->velocity = ((pivots[i] ->velocity)/max)*0.35;
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

void Driver::pub_telemetry() {

    // Construct a message from our current is_autonomous boolean
    core::msg::Telemetry msg;

    for(PivotModule *pivot : pivots) {

        core::msg::SingleTelemetry wheel_msg;
        core::msg::SingleTelemetry pivot_msg;

        // Get the telemetry from the wheel and pivot
        BLCMDTelemetry wheel_tel;
        pivot->cmdWheel->get_telemetry(&wheel_tel, ROSTimers::blcmds_telemetry);
        BLCMDTelemetry pivot_tel;
        pivot->cmdPivot->get_telemetry(&pivot_tel, ROSTimers::blcmds_telemetry);

        // Fill the wheel message from wheel telemetry
        wheel_msg.bus = this->get_parameter("canbus").get_parameter_value().get<std::string>();
        wheel_msg.id = pivot->cmdWheel->get_id();
        wheel_msg.rotor_velocity = wheel_tel.rotor_velocity;
        wheel_msg.q_current = wheel_tel.q_current;
        wheel_msg.rotor_interval = wheel_tel.rotor_interval;
        wheel_msg.d_current = wheel_tel.d_current;
        wheel_msg.resolver_position = wheel_tel.resolver_position;
        wheel_msg.resolver_velocity = wheel_tel.resolver_velocity;
        wheel_msg.power = wheel_tel.power;
        wheel_msg.voltage = wheel_tel.voltage;
        wheel_msg.temperature = wheel_tel.temp;

        // Fill the pivot message from pivot telemetry
        pivot_msg.bus = this->get_parameter("canbus").get_parameter_value().get<std::string>();
        pivot_msg.id = pivot->cmdPivot->get_id();
        pivot_msg.rotor_velocity = pivot_tel.rotor_velocity;
        pivot_msg.q_current = pivot_tel.q_current;
        pivot_msg.rotor_interval = pivot_tel.rotor_interval;
        pivot_msg.d_current = pivot_tel.d_current;
        pivot_msg.resolver_position = pivot_tel.resolver_position;
        pivot_msg.resolver_velocity = pivot_tel.resolver_velocity;
        pivot_msg.power = pivot_tel.power;
        pivot_msg.voltage = pivot_tel.voltage;
        pivot_msg.temperature = pivot_tel.temp;

        // Push the messages into the telemetry message
        msg.wheels.push_back(wheel_msg);
        msg.pivots.push_back(pivot_msg);
    }

    // publish the message
    telemetry_pub->publish(msg);
}

// Main constructor that sets up the node
Driver::Driver() : Node("driver")
{

    this->declare_parameter("canbus", "can0");
    // parameter for change in angle of the pivots in radians per second
    this->declare_parameter("max-theta", M_PI_2);
    d_theta = this->get_parameter("max-theta").get_parameter_value().get<double>()
            *ROSTimers::drive_control.count()/1000;

    // Output set-up messages
    Print::title("DRIVER");
    Print::print("", true);

    // Initialise the wheels in the correct direction
    for (size_t i = 0; i < NUM_WHEELS; i++)
    {
        bool left = i < 2;
        BLCMD *cmdWheel = new BLCMD(this->get_parameter("canbus").get_parameter_value().get<std::string>(),
                    i + 1, DRIVE_VELOCITY, left, STOP);
        BLCMD *cmdPivot = new BLCMD(this->get_parameter("canbus").get_parameter_value().get<std::string>(),
                   i + 5, DRIVE_POSITION, !(i%2), STOP);
        pivots[i] = new PivotModule(i, cmdWheel, cmdPivot, i%2 ? angle_offset : -angle_offset);
        pivots[i]->cmdPivot->drive(pivots[i]->angle);

    }

    rclcpp::QoS qos = rclcpp::QoS(1).best_effort().deadline(ROSTimers::drive_deadline);

    rclcpp::SubscriptionOptions subscriber_options;

    // Sets subscriber options before subscription is made
    subscriber_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void
    { inputs_deadline_exceeded(); };


    subscription_cmds_man = this->create_subscription<core::msg::DriveInput>(
        "/control/drive_inputs", qos, std::bind(&Driver::drive_callback, this, _1), subscriber_options);

    // Creates the commands subscription (autonomous)
    subscription_cmds_auto = this->create_subscription<core::msg::DriveInput>(
        "/autonomous/drive_inputs", 10, std::bind(&Driver::auto_callback, this, _1));

    // Creates the input subscription
    subscription_inputs = this->create_subscription<core::msg::InputGamepad>(
        "/control/input_gamepad", qos, std::bind(&Driver::input_callback, this, _1), subscriber_options);
;
    // Creates auto mode timer and associated publisher
    mode_timer = this->create_wall_timer(ROSTimers::auto_mode, std::bind(&Driver::pub_auto_mode, this));

    //telemetry_timer = this->create_wall_timer(ROSTimers::blcmds_telemetry, std::bind(&Driver::pub_telemetry, this));

    mode_pub = this->create_publisher<std_msgs::msg::Bool>(
        "/autonomous/mode", 10);

    //telemetry_pub = this->create_publisher<core::msg::Telemetry>("/control/telemetry", 10);

    pivot_wheel_pub = this->create_publisher<core::msg::PivotWheelData>("/control/pivot_wheel", 10);

}

// deadline callback for when the drive inputs publisher misses its deadline
void Driver::inputs_deadline_exceeded()
{
    RCLCPP_WARN(this->get_logger(), "Drive inputs subscriber deadline missed");
    for (PivotModule *pivot : pivots)
    {
        pivot->cmdWheel->stop();
    }
}

//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Subscriber class
    rclcpp::spin(std::make_shared<Driver>());

    rclcpp::shutdown();
    // Returns an empty value
    return 0;


}
