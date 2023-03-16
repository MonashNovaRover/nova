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
    if (blcmd_error) return;

    core::msg::PivotWheelData data_msg;
    if (!msg->strafe_mode)
    {
        // Find the turning radius form the 'steer' command
        // This will find the valid radius that the wheels can turn to based on max speed of pivots
        double radius = get_turning_radius(msg->steer);

        // Fill the wheel angles and velocities
        fill_wheel_angles_radial(radius);
        fill_wheel_velocities_radial(msg->speed * get_parameter("max-speed").get_parameter_value().get<double>(), radius);
        data_msg.radius = radius;
    }
    else if (msg->strafe_mode)
    {
        fill_wheel_angles_strafe();
        fill_wheel_velocities_strafe(msg->speed * get_parameter("max-speed").get_parameter_value().get<double>());
        data_msg.radius = 0;
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

    data_msg.steer = msg->steer;
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
            pivot->cmdWheel->set_stop_mode(STOP);
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
            pivot->cmdWheel->set_stop_mode(DRIVE_VELOCITY);
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

void Driver::blcmd_status_callback(const core::msg::BLCMDStatusArray::SharedPtr msg) {
    bool error = false;
    for(core::msg::BLCMDStatus status : msg->blcmds) {
        error = error || status.gate_fault || status.stall_fault || status.resolver_fault;
    }
    blcmd_error = error;
}

// Gets the turning radius of the rover
double Driver::get_turning_radius(float steer)
{
    double radius = steer == 0 ? INFINITY : (1.0 / steer) - (steer > 0 ? 1 : -1);

    // If steer is 0, get the old sign, otherwise get the new sign
    int new_sign = steer == 0 ? sign : (steer > 0 ? 1 : -1);

    //update the sign
    sign = new_sign;

    //set initial maximum change and index
    double max_change = -INFINITY;
    int index = 0;

    //Find the wheel that has to turn the most to get to the target
    for (size_t i = 0; i < NUM_WHEELS; i++)
    {
        double target = calc_wheel_angle(radius, i, sign);
        // Get the maximum change in angle
        if (abs(target - pivots[i]->angle) > max_change) {
            max_change = abs(target - pivots[i]->angle);
            index = i;
        }
    }

    double target = calc_wheel_angle(radius, index, sign);

    //determine the direction this wheel has to turn
    int direction = pivots[index]->angle == target ? 0 : (pivots[index]->angle < target ? 1 : -1);
    //calculate the maximum angle the wheel can turn until the next drive command is recieved.
    d_theta = this->get_parameter("max-theta").get_parameter_value().get<double>()
              *ROSTimers::drive_control.count()/1000;
    double theta = pivots[index]->angle + direction * d_theta;
    //if the target is closer than the maximum angle the wheel can turn, set the angle to the target
    if (abs(theta - target) < d_theta) theta = target;
    // return the radius of the circle the wheel is turning to
    return radius_from_angle(theta, index, sign);
}

double Driver::calc_wheel_angle(float radius, int wheel, int sign)
{
    double angle;
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
        default:
            angle = 0;
    }

    return angle;

}

double Driver::radius_from_angle(double angle, int wheel, int sign) {

    double radius;

    //if the absolute value of the angle is greater than 90 degrees, radius is 0
    if (abs(angle) >= M_PI_2) return 0;

    // inverse of the math in calc_wheel_angle
    switch(wheel){
        case 0:
            radius = (angle == -angle_offset ? INFINITY : (tan(angle + angle_offset + sign*M_PI_2) * CHASSIS_LENGTH/2 - CHASSIS_WIDTH/2));
            break;
        case 1:
            radius = (angle == angle_offset ? INFINITY : (tan(angle - angle_offset - sign*M_PI_2) * -CHASSIS_LENGTH/2 - CHASSIS_WIDTH/2));
            break;
        case 2:
            radius = (angle == -angle_offset ? INFINITY : (tan(angle + angle_offset - sign*M_PI_2) * -CHASSIS_LENGTH/2 + CHASSIS_WIDTH/2));
            break;
        case 3:
            radius = (angle == angle_offset ? INFINITY : (tan(angle - angle_offset + sign*M_PI_2) * CHASSIS_LENGTH/2 + CHASSIS_WIDTH/2));
            break;
        default:
            radius = INFINITY;
    }
    return radius;
}


void Driver::fill_wheel_angles_radial(double radius)
{
    //if radius is 0, get the old sign, otherwise get sign of the radius
    int curr_sign = radius == 0 ? sign : (radius > 0 ? 1 : -1);
    for(int i = 0; i < NUM_WHEELS; i++)
    {
        pivots[i]->angle = calc_wheel_angle(radius, i, curr_sign);
    }
}

void Driver::fill_wheel_velocities_radial(float speed, float radius)
{
    //calculate the ration of the left and right wheels
    float left_ratio = !radius ? 1 : sqrt(pow(CHASSIS_LENGTH, 2.0)/4 + pow(radius + (CHASSIS_WIDTH / 2), 2.0))/abs(radius);
    float right_ratio = !radius ? 1 : sqrt(pow(CHASSIS_LENGTH, 2.0)/4 + pow(radius - (CHASSIS_WIDTH / 2), 2.0))/abs(radius);

    float max_ratio = max(abs(left_ratio), abs(right_ratio));

    for (size_t i = 0; i < NUM_WHEELS; i++)
    {
        //if the radius is infinity (going straight) all wheels speeds are equal. Otherwise multiply by the ratio
        // of the relevant side and divide by the maximum ratio
        if (i < 2) //left wheels
        {
            pivots[i]->velocity = radius == INFINITY ? speed : speed*left_ratio/max_ratio;
        }
        else //right wheels
        {
            pivots[i]->velocity =  radius == INFINITY ? speed : speed*right_ratio/max_ratio;
        }
    }
}

void Driver::fill_wheel_angles_strafe() {
    for (size_t i = 0; i < NUM_WHEELS; i++) {
        //diagonals have the same angles as each other and negative x/y neighbors
        pivots[i]->angle = (i%2 ? -1 : 1) * (M_PI_2 - angle_offset);
    }
}

void Driver::fill_wheel_velocities_strafe(float speed) {
    for (size_t i = 0; i < NUM_WHEELS; i++) {
        //diagonals have the same direction as each other and negative x/y neighbors
        pivots[i]->velocity = speed * (i%2 ? -1 : 1);
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
        BLCMDTelemetry wheel_tel = pivot->cmdWheel->get_telemetry();
        BLCMDTelemetry pivot_tel = pivot->cmdPivot->get_telemetry();

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
    // parameter for max velocity of the wheels, all speeds received from /control/drive_inputs are scaled by this value
    this->declare_parameter("max-speed", 0.35);


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
        //pivots[i]->cmdPivot->drive(pivots[i]->angle);

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

    subscription_blcmd_status = this->create_subscription<core::msg::BLCMDStatusArray>(
        "/control/blcmd_status", 10, std::bind(&Driver::blcmd_status_callback, this, _1));
;
    // Creates auto mode timer and associated publisher
    mode_timer = this->create_wall_timer(ROSTimers::auto_mode, std::bind(&Driver::pub_auto_mode, this));

    telemetry_timer = this->create_wall_timer(ROSTimers::blcmds_telemetry, std::bind(&Driver::pub_telemetry, this));

    //Create blcmd spin timer

    blcmd_spin_timer = this->create_wall_timer(ROSTimers::blcmd_spin, std::bind(&Driver::blcmd_spinner, this));

    mode_pub = this->create_publisher<std_msgs::msg::Bool>(
        "/autonomous/mode", 10);

    telemetry_pub = this->create_publisher<core::msg::Telemetry>("/control/telemetry", 10);

    pivot_wheel_pub = this->create_publisher<core::msg::PivotWheelData>("/control/pivot_wheel", 10);

}

void Driver::blcmd_spinner() {
    // spin the wheel and pivot blcmds
    for (PivotModule *pivot : pivots) {
        pivot->cmdWheel->spin();
        pivot->cmdPivot->spin();
    }
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
