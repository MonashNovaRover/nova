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
void Driver::send_commands()
{
    core::msg::PivotWheelData data_msg;
    switch (mode) {
        case core::msg::DriveInput::PIVOT: {
            // Find the turning radius form the 'steer' command
            // This will find the valid radius that the wheels can turn to based on max speed of pivots
            RCLCPP_DEBUG(this->get_logger(), "target radius: %f", target_radius);
            RCLCPP_DEBUG(this->get_logger(), "target direction: %d", target_direction);
            set_best_effort_radius();
            RCLCPP_DEBUG(this->get_logger(), "new best effort radius: %f", best_effort_radius);
            RCLCPP_DEBUG(this->get_logger(), "new best effort direction: %d", best_effort_direction);

            // Fill the wheel angles and velocities
            fill_wheel_angles_radial();
            fill_wheel_velocities_radial(velocity * get_parameter("max_speed").get_parameter_value().get<double>(),
                                         best_effort_radius);
            data_msg.radius = best_effort_radius;
            break;
        }
        case core::msg::DriveInput::STRAFE: {
            fill_wheel_angles_strafe();
            fill_wheel_velocities_strafe(velocity * get_parameter("max_speed").get_parameter_value().get<double>());
            data_msg.radius = 0;
            break;
        }

        case core::msg::DriveInput::TANK: {
            fill_wheel_velocities_tank(velocity * get_parameter("max_speed").get_parameter_value().get<double>(),
                    target_radius, target_direction);
            data_msg.radius = target_radius;
            break;
        }
    }

    // Send velocities to the wheels
    for (size_t i = 0; i < NUM_WHEELS; i++)
    {
        PivotModule *pivot = pivots[i];
        pivot->cmdWheel->drive(pivot->velocity);
        if (mode == core::msg::DriveInput::PIVOT || mode == core::msg::DriveInput::STRAFE) {
            pivot->cmdPivot->drive(pivot->angle);
        }
        data_msg.angles[i] = pivot->angle;
        data_msg.velocities[i] = pivot->velocity;
    }

    pivot_wheel_pub->publish(data_msg);
}

// Receives drive commands
void Driver::drive_callback(const core::msg::DriveInput::SharedPtr msg)
{
    // Set the mode
    mode = msg->mode;
    // Set the speed and radius
    float prev_velocity = velocity;
    velocity = msg->speed;
    float d_vel = velocity - prev_velocity;
    if (abs(d_vel) > max_d_vel) {
        velocity = prev_velocity + max_d_vel * (d_vel > 0 ? 1 : -1);
    };
    target_radius = msg->radius;
    target_direction = msg->direction;
    handbrake = msg->handbrake;
}

// Gets the turning radius of the rover
void Driver::set_best_effort_radius() {

    bool left;
    //Find the wheel that has to turn the most to get to the target
    double left_angle = calc_wheel_angle(target_radius, true, target_direction);
    double right_angle = calc_wheel_angle(target_radius, false, target_direction);
    RCLCPP_DEBUG(this->get_logger(), "target: left angle: %f, right_angle: %f", left_angle, right_angle);
    RCLCPP_DEBUG(this->get_logger(), "current: left angle: %f, right_angle: %f", pivots[0]->angle, pivots[3]->angle);
    left = abs(left_angle - pivots[0]->angle) >= abs(right_angle - pivots[3]->angle);
    RCLCPP_DEBUG(this->get_logger(), "furthest side: %s", left ? "left" : "right");
    int index = left ? 0 : 3;
    double target = left ? left_angle : right_angle;
    //determine the direction this wheel has to turn
    int drive_dir = pivots[index]->angle == target ? 0 : (pivots[index]->angle < target ? 1 : -1);
    RCLCPP_DEBUG(this->get_logger(), "drive direction: %d", drive_dir);
    //calculate the maximum angle the wheel can turn until the next drive command is recieved.
    double best_effort_angle = pivots[index]->angle + drive_dir * max_d_theta;
    //if the target is closer than the maximum angle the wheel can turn, set the angle to the target
    if (abs(best_effort_angle - target) < max_d_theta) best_effort_angle = target;
    RCLCPP_DEBUG(this->get_logger(), "best effort angle: %f", best_effort_angle);
    if(left) best_effort_direction = best_effort_angle > -angle_offset ? -1 : 1;
    else best_effort_direction = best_effort_angle > -angle_offset ? 1 : -1;
    // return the radius of the circle the wheel is turning to
    best_effort_radius = radius_from_angle(best_effort_angle, left);
}

double Driver::calc_wheel_angle(float radius, bool left, int dir)
{
    double angle;
    // gradient of line https://www.desmos.com/calculator/opj8exj9gp

    // only need to consider left and right wheel angles as they are the same angles but opposite direction, and
    // front and back wheels are driven in opposite directions by blcmd boards
    //TODO: Verify math
    if(left){
        angle = (radius == INFINITY ? 0 : atan((2*radius*dir + CHASSIS_WIDTH)/CHASSIS_LENGTH) - dir * M_PI_2) - angle_offset;
    } else {
        angle = -(radius == INFINITY ? 0 : atan((2*radius*dir - CHASSIS_WIDTH)/CHASSIS_LENGTH) - dir * M_PI_2) - angle_offset;
    }
    return angle;
}

double Driver::radius_from_angle(double angle, bool left) {

    double radius;

    //if the absolute value of the angle is greater than 90 degrees, radius is 0
    if (abs(angle) >= M_PI_2) return 0;

    // inverse of the math in calc_wheel_angle
    if(left){
        int dir = angle < -angle_offset ? 1 : -1;
        radius = (angle == -angle_offset ? INFINITY : (tan(angle + angle_offset + M_PI_2) * CHASSIS_LENGTH - CHASSIS_WIDTH)/(2*dir));
    } else {
        int dir = angle < - angle_offset ? -1 : 1;
        radius = (angle == -angle_offset ? INFINITY : (tan(-angle - angle_offset + M_PI_2) * CHASSIS_LENGTH + CHASSIS_WIDTH)/(2*dir));
    }
    return radius;
}


void Driver::fill_wheel_angles_radial()
{
    //if radius is 0, get the old sign, otherwise get sign of the radius
    //int curr_sign = radius == 0 ? sign : (radius > 0 ? 1 : -1);
    for(int i = 0; i < NUM_WHEELS; i++)
    {
        pivots[i]->angle = calc_wheel_angle(best_effort_radius, i<2, best_effort_direction);
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

void Driver::fill_wheel_velocities_tank(float speed, float radius, float dir) {
    if (radius == INFINITY) {
        for (size_t i = 0; i < NUM_WHEELS; i++){
            pivots[i]->velocity = speed;
        }
    }
    else {
        // Calculate distances from the wheelbase centre to each wheel, and the maximum distance
        float distances[NUM_WHEELS];
        float max_distance = 0;
        for (size_t i = 0; i < NUM_WHEELS; i++) {
            Vector2 position = get_wheel_position(pivots[i]->cmdWheel->get_id());
            distances[i] = get_wheel_distance(position, radius);
            if (distances[i] > max_distance) max_distance = distances[i];
        }

        // Fill wheel velocities, scaling each by its distance to the wheelbase centre
        // Approximating that the wheels drive tangent to the turning circle, the scaling ensures
        // each wheel achieves the same angular velocity about the turning center the rover
        for (size_t i = 0; i < NUM_WHEELS; i++) {
            pivots[i]->velocity = speed * distances[i] / max_distance;
        }

        // Modify wheel directions if the turning centre is under the rover wheelbase
        // Ignore the edge case where the turning centre is exactly below the centre of a wheel.
        // Does not affect the behaviour in practice
        float wheel_x = CHASSIS_WIDTH / 2.0;
        if (abs(radius) < wheel_x) {
            // If the turning centre is...
            if (radius > -wheel_x && radius <= 0 && dir < 0) {
                // Under the left half of the chassis, reverse the left wheels
                // Also include cases where we are pivoting left
                pivots[0]->velocity *= -1;
                pivots[1]->velocity *= -1;
            } else if (radius >= 0 && radius < wheel_x && dir > 0) {
                // Under the right half of the chassis, reverse the right wheels
                // Also include cases where we are pivoting right
                pivots[2]->velocity *= -1;
                pivots[3]->velocity *= -1;
            }
        }
    }
}

Vector2 Driver::get_wheel_position (int id) {

    // Determine the y position
    float y = 0;
    if (id == 1 || id == 4) y = CHASSIS_LENGTH/2;
    else if (id == 2 || id == 3) y = -CHASSIS_LENGTH/2;

    // Determine the x position
    float wheel_x = (CHASSIS_WIDTH / 2.0) * ((id <= 2) ? -1.0 : 1.0);

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
    this->declare_parameter("max_theta", M_PI_2);
    max_d_theta = this->get_parameter("max_theta").get_parameter_value().get<double>()*
            ROSTimers::drive_control.count()/1000;
    // parameter for max velocity of the wheels, all speeds received from /control/drive_inputs are scaled by this value
    this->declare_parameter("max_acceleration", 1.0);
    max_d_vel = this->get_parameter("max_acceleration").get_parameter_value().get<double>()*
            ROSTimers::drive_control.count()/1000;
    this->declare_parameter("max_speed", 0.9);
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
                   i + 5, DRIVE_POSITION, false, STOP);
        pivots[i] = new PivotModule(i, cmdWheel, cmdPivot, -angle_offset);
        //pivots[i]->cmdPivot->drive(pivots[i]->angle);

    }

    rclcpp::QoS qos = rclcpp::QoS(1).best_effort().deadline(ROSTimers::drive_deadline);

    rclcpp::SubscriptionOptions subscriber_options;

    // Sets subscriber options before subscription is made
    subscriber_options.event_callbacks.deadline_callback = [this](rclcpp::QOSDeadlineRequestedInfo) -> void
    { drive_inputs_deadline_exceeded(); };

    subscription_cmds_man = this->create_subscription<core::msg::DriveInput>(
        "/control/drive_inputs", qos, std::bind(&Driver::drive_callback, this, _1), subscriber_options);

    // Create send commands timer
    send_commands_timer = this->create_wall_timer(ROSTimers::drive_control, std::bind(&Driver::send_commands, this));

    // Create blcmds telemetry timer
    telemetry_timer = this->create_wall_timer(ROSTimers::blcmds_telemetry, std::bind(&Driver::pub_telemetry, this));

    //Create blcmd spin timer
    blcmd_spin_timer = this->create_wall_timer(ROSTimers::blcmd_spin, std::bind(&Driver::blcmd_spinner, this));

    // Create telemetry publisher
    telemetry_pub = this->create_publisher<core::msg::Telemetry>("/control/telemetry", 10);

    // Create pivot wheel data publisher
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
void Driver::drive_inputs_deadline_exceeded()
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
