/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Alexander Li
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "spm_kinematics.h"
#include "arm_core.h"
#include "print/print.h"
#include <cmath>
#define _USE_MATH_DEFINES

SpmKinematics::SpmKinematics() : Node("spm_kinematics")
{
    //Initialise constants - 
    joint_velocities_timer_period = 50ms; //this is the time_since_last_publish, used in integrator and differentiator
    time_since_last_publish = 50; //in milliseconds
    cmd_outputs_timer_period = 50ms;
    num_base_joints = 3; // number of joints before the spm wrist joints
    current_rpy_pos = {0, 0, 0}; //initialise current rpy joint positions
    //SPM geometry
    beta = 54.74 * M_PI / 180;
    gamma = 54.74 * M_PI / 180;
    alpha[0] = M_PI/2;
    alpha[1] = M_PI/2;
    alpha[2] = 2 * asin(sin(beta) * cos(M_PI/6));
    eng[0] = 0;
    eng[1] = 2 * M_PI / 3;
    eng[2] = 4 * M_PI / 3;


    // Create subscription to resolvers
    resolver_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/electronics/resolvers", 10, std::bind(&SpmPositionKinematics::resolver_callback, this, _1)
    );

    // Create subscription to desired joint velocities
    joint_velocities_sub = this->create_subscription<sensor_msgs::msg::JointState>(
        "/control/joint_velocities", 10, std::bind(&SpmPositionKinematics::joint_velocities_callback, this, _1)
    );

    // Create timer and publisher for cmd_outputs
    cmd_outputs_timer = this->create_wall_timer(
        cmd_outputs_timer_period, std::bind(&SpmPositionKinematics::publish_cmd_outputs, this)
    );
    cmd_outputs_pub = this->create_publisher<sensor_msgs::msg::JointState>(
        "/control/cmd_outputs", rclcpp::QoS(1).best_effort().deadline(200ms)
    );

    // Output set-up messages
    Print::title("SPM KINEMATICS");
    Print::print("Subscribed Topics:");
    Print::print("/electronics/resolvers            [sensor_msgs/JointState]", 1);
    Print::print("/control/joint_velocities         [sensor_msgs/JointState]", 1);
    Print::print("Published Topics:");
    Print::print("/control/cmd_outputs              [sensor_msgs/JointState]", 1);
    Print::print("", true);
}

// Update current wrist joint positions, using resolvers
void SpmKinematics::resolver_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    for (unsigned int i = 0; i < 3; i++) {
        current_wrist_joint_pos[i] = msg.position[num_base_joints+i];
    }
}

// Update desired rpy velocities, using control/joint_velocitites
void SpmKinematics::joint_velocities_callback(const sensor_msgs::msg::JointState::SharedPtr msg)
{
    for (unsigned int i = 0; i < 3; i++) {
        desired_rpy_vel[i] = msg.velocity[num_base_joints+i];
    }
}

// Main solver
void SpmKinematics::solve()
{
    // obtain current rpy
    previous_rpy_pos = current_rpy_pos;
    current_rpy_pos = spm_fk(current_wrist_joint_pos);
    // integrate
    desired_rpy_pos = rpy_integrator(current_rpy_pos, desired_rpy_vel, time_since_last_publish);
    // ik
    desired_wrist_joint_pos = spm_ik(desired_rpy_pos);
    // differentiate
    desired_wrist_joint_vel = joint_differentiator(current_wrist_joint_pos, desired_wrist_joint_pos, time_since_last_publish);
}

//perform spm position fk
float *SpmKinematics::spm_fk(const float wrist_joint_pos[3])
{
    //TODO: implement simultaneous equation solver
    float v[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};

    //solve fk, obtain v1, v2, v3, store into an array of v[9] = {v1x v1y v1z v2x v2y v2z v3x v3y v3z}


    //convert v vectors to rpy
    return v_to_rpy(v, previous_rpy_pos);
}

// perform spm position ik
float *SpmKinematics::spm_ik(const float desired_rpy_pos[3])
{
    static float theta[3];
    // convert rpy to v
    float *v = rpy_to_v(desired_rpy_pos);

    // solve for theta
    for (unsigned int i = 0; i < 3; i++) {
        float A = (-sin(eng[i])*sin(gamma)*cos(alpha[i])+sin(eng[i])*cos(gamma)*sin(alpha[i])) * (-v[3*i+0])
            + (cos(eng[i])*sin(gamma)*cos(alpha[i])-cos(eng[i])*cos(gamma)*sin(alpha[i])) * v[3*i+1]
            + (cos(gamma)*cos(alpha[i])-sin(gamma)*sin(alha[i])) * v[3*i+2] - cos(alpha[1]);
        float B = 2 * (cos(eng[i])*sin(alpha[i]) * (-v[3*i+0]) + sin(eng[i])*sin(alpha[i]) * v[3*i+1]);
        float C = (-sin(eng[i])*sin(gamma)*cos(alpha[i])-sin(eng[i])*cos(gamma)*sin(alpha[i])) * (-v[3*i+0])
            + (cos(eng[i])*sin(gamma)*cos(alpha[i])+cos(eng[i])*cos(gamma)*sin(alpha[i])) * v[3*i+1]
            + (-cos(gamma)*cos(alpha[i])+sin(gamma)*sin(alha[i])) * v[3*i+2] - cos(alpha[1]);
        // take positive root
        float T = (-B + sqrt(B*B - 4*A*C)) / (2*A);
        theta[i] = 2 * atan(T);
    }

    return theta;
}


// integrate rpy vel into rpy pos
float *SpmKinematics::rpy_integrator(const float current_rpy[3], const float desired_rpy_vel[3], int time_step) 
{
    static float rpy_pos[3];
    for (unsigned int i = 0; i < 3; i++) {
        rpy_pos[i] = current_rpy[i] + desired_rpy_vel[i] * time_step;
    }

    return rpy_pos;
}

// differentiate desired joint angles into desired joint velocities
float *SpmKinematics::joint_differentiator(const float current_wrist_joint_pos[3], const float desired_wrist_joint_pos[3], int time_step)
{
    static float joint_v[3];
    for (unsigned int i = 0; i < 3; i++) {
        joint_v[i] = (desired_wrist_joint_pos[i] - current_wrist_joint_post[i]) / time_step;
    }
    return joint_v;
}

// convert v vectors to rpy
float *SpmKinematics::v_to_rpy(float v[9], float prev_rpy[3])
{
    // extract v vectors
    float v1[3] = {v[0], v[1], v[2]};
    float v2[3] = {v[3], v[4], v[5]};
    float v3[3] = {v[6], v[7], v[8]};

    // convert to axes of output frame
    float *z = vector_scalar_product(vector_addition(v1, v2, v3), sqrt(vector_dot_product(vector_addition(v1, v2, v3))));
    float *x = vector_scalar_product(vector_addition(v2, vector_scalar_product(z, -cos(beta))), 1/sin(beta));
    float *y = vector_cross_product(z, x);

    // set up solutions
    float theta_a[3];
    float theta_b[3];
    // finding 2 possible theta_y
    float theta_a[1] = asin(z[0]);
    float theta_b[1] = M_PI - theta_y_a;
    // finding corresponding angles of theta_x
    float theta_a[0] = atan2(-z[1]/cos(theta_y_a), z[2]/cos(theta_y_a));
    float theta_b[0] = atan2(-z[1]/cos(theta_y_b), z[2]/cos(theta_y_b));
    // finding corresponding angles of theta_z
    float theta_a[2] = atan2(-y[0]/cos(theta_y_a), x[0]/cos(theta_y_a));
    float theta_b[2] = atan2(-y[0]/cos(theta_y_b), x[0]/cos(theta_y_b));
    // choose between the solutions; select the one with smaller metric to previous Euler configuration
    static float rpy[3] = {0, 0, 0};
    if (euler_metric(theta_a, prev_rpy) < euler_metric(theta_b, prev_rpy)) {
        rpy[0] = theta_a[0];
        rpy[1] = theta_a[1];
        rpy[2] = theta_a[2];
    } else {
        rpy[0] = theta_b[0];
        rpy[1] = theta_b[1];
        rpy[2] = theta_b[2];
    }
    return rpy;
}

// convert rpy to v vectors
float *SpmKinematics::rpy_to_v(float rpy[3])
{
    float theta_x = rpy[0];
    float theta_y = rpy[1];
    float theta_z = rpy[2];
    
    // convert into the axes of output frame
    float x[3] = {
        cos(theta_z) * cos(theta_y),
        cos(theta_z) * sin(theta_y) * sin(theta_x) + sin(theta_z) * cos(theta_x),
        -cos(theta_z) * sin(theta_y) * cos(theta_x) + sin(theta_z) * sin(theta_x)
    };
    float y[3] = {
        -sin(theta_z) * cos(theta_y),
        -sin(theta_z) * sin(theta_y) * sin(theta_x) + cos(theta_z) * cos(theta_x), 
        sin(theta_z) * sin(theta_y) * cos(theta_x) + cos(theta_z) * sin(theta_x)
    };
    float z[3] = {
        sin(theta_y),
        -cos(theta_y) * sin(theta_x),
        cos(theta_y) * cos(theta_x)
    };

    // find v2
    float *v2 = vector_addition(vector_scalar_product(x, sin(beta)), vector_scalar_product(z, cos(beta)));

    // find p1, p3
    float *p1 = vector_addition(vector_scalar_product(x, cos(2 *M_PI/3)), vector_scalar_product(y, sin(2 *M_PI/3)));
    float *p3 = vector_addition(vector_scalar_product(x, cos(-2 *M_PI/3)), vector_scalar_product(y, sin(-2 *M_PI/3)));

    //find v1, v3
    float *v1 = vector_addition(vector_scalar_product(p1, sin(beta)), vector_scalar_product(z, cos(beta)));
    float *v3 = vector_addition(vector_scalar_product(p3, sin(beta)), vector_scalar_product(z, cos(beta)));

    //return v vectors as a single array
    static float v[9] = {v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], v3[0], v3[1], v3[2]};
    return v;
}


// vector addition for 2 vectors
float *vector_addition(float x[3], float y[3])
{
    static float output[3];
    for (unsigned int i = 0; i < 3; i++) {
        output[i] = x[i] + y[i];
    }
    return output;
}

// vector addition for 3 vectors
float *vector_addition(float x[3], float y[3], float z[3])
{
    static float output[3];
    for (unsigned int i = 0; i < 3; i++) {
        output[i] = x[i] + y[i] + z[i];
    }
    return output;
}

// vector scalar product of length 3
float *vector_scalar_product(float v[3], float s)
{
    static float output[3];
    for (unsigned int i = 0; i < 3; i++) {
        output[i] = v[i] * s;
    }
    return output;
}

//  dot product of vectors of length 3
float vector_dot_product(float x[3], float y[3])
{
    float output = 0;;
    for (unsigned int i = 0; i < 3; i++) {
        output += x[i] * y[i];
    }
    return output;
}

// cross product of vectors of length 3
float *vector_cross_product(float x[3], float y[3])
{
    static float z[3];
    z[0] = x[1] * y[2] - x[2] * y[1];
    z[1] = x[2] * y[0] - x[0] * y[2];
    z[2] = x[0] * y[1] - x[1] * y[0];
    return z;
}

// custom metric for euler angles
float euler_metric(float theta_a[3], float theta_b[3])
{
    float dist_x = abs(theta_a[0] - theta_b[0]);
    if (dist_x > 180.0) {dist_x -= 180;}
    float dist_y = abs(theta_a[1] - theta_b[1]);
    if (dist_y > 180.0) {dist_y -= 180;}
    float dist_z = abs(theta_a[2] - theta_b[2]);
    if (dist_z > 180.0) {dist_z -= 180;}
    
    return sqrt(dist_x * dist_x + dist_y * dist_y + dist_z * dist_z);
}

//  Main function called when the script execution begins
int main(int argc, char **argv)
{
    // Initialises the ROS C++ class
    rclcpp::init(argc, argv);

    // Runs the Publisher class
    rclcpp::spin(std::make_shared<SpmKinematics>());

    // Shutsdown ROS once complete
    rclcpp::shutdown();

    // Returns an empty value
    return 0;
}
