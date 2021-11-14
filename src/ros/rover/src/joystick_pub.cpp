#include <iostream>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <linux/joystick.h>
#include <chrono>
#include <functional>
#include <memory>
#include <string>

//--**-- General includes
#include <gamepad/gamepad.h>

#define LOOP_HZ 10
#include "joystick.h"

// Include ROS packages
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_gamepad.hpp"

using namespace std;
using namespace std::chrono_literals;

class JoystickPublisher : public rclcpp::Node {
    public:

    void check_input () {
        // Updates the state of the gamepad
		GamepadUpdate();

		//update the status of each controller
		xbox->update();
        publisher_->publish(xbox->getMessage());
    }

    JoystickPublisher() : Node("joystick_pub"), count_(0)
    {
        GamepadInit();
        xbox = new Joystick(GAMEPAD_0);     
        publisher_ = this->create_publisher<core::msg::InputGamepad>("/control/input_gamepad", 10);
        timer_ = this->create_wall_timer(10ms, std::bind(&JoystickPublisher::check_input, this));
    }

    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Publisher<core::msg::InputGamepad>::SharedPtr publisher_;
    size_t count_;
    Joystick* xbox;
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<JoystickPublisher>());
    rclcpp::shutdown();
    return 0;
    /*
	// ros publisher code
	// CONFUSED
	ros::init(argc, argv, "joystick_input");
	ros::NodeHandle n;
	ros::Rate loop_rate(LOOP_HZ);

	// Intialise the gamepad library
	GamepadInit();

	// construct each controller instance
	Joystick xbox(GAMEPAD_0);
	Joystick ljs(GAMEPAD_1, 0.435);
	Joystick rjs(GAMEPAD_2, 0.435);

	ros::Publisher xbox_raw_ctrl = n.advertise<common::RawCtrl>("/base/xbox_raw_ctrl", 1);
	ros::Publisher ljs_raw_ctrl = n.advertise<common::RawCtrl>("/base/ljs_raw_ctrl", 1);
	ros::Publisher rjs_raw_ctrl = n.advertise<common::RawCtrl>("/base/rjs_raw_ctrl", 1);

	//ros ok loop.
	while (ros::ok())
	{
		// Updates the state of the gamepad
		GamepadUpdate();

		//update the status of each controller
		xbox.update();
		ljs.update();
		rjs.update();

		// publish the ros messages
		xbox_raw_ctrl.publish(xbox.getMessage());
		ljs_raw_ctrl.publish(ljs.getMessage());
		rjs_raw_ctrl.publish(rjs.getMessage());

		ros::spinOnce();
		loop_rate.sleep();
	}*/
}