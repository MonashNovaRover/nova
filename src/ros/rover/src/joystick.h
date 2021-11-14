#ifndef JOYSTICK_H
#define JOYSTICK_H
//--**-- ROS includes
#include "rclcpp/rclcpp.hpp"
#include "core/msg/input_gamepad.hpp"

//--**-- General includes
#include <gamepad/gamepad.h>

#include <cmath>

using namespace std;

/*
 *--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--
 * JoystickClass:
 *    Handles Joystick inputs for rover driving and arm control
 *    Uses gamepad library to handle joystick inputs
 *    Stores inputs in RawCtrl custom message object.
 *--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--**--..--
 */
class Joystick {

	public:
		Joystick(GAMEPAD_DEVICE controller);
		Joystick(GAMEPAD_DEVICE controller, float offset);
		void update();
		core::msg::InputGamepad getMessage();
		int sgn(float val);
	protected:
		core::msg::InputGamepad msg_;
		GAMEPAD_DEVICE controller_;

		const float STICK_MAX_L_ = 32767 - GAMEPAD_DEADZONE_LEFT_STICK;
        const float STICK_MAX_R_ = 32767 - GAMEPAD_DEADZONE_RIGHT_STICK;

        float offset_;
		int stick_lx_;
		int stick_ly_;
		int stick_rx_;
		int stick_ry_;

		float stick_lx_f;
		float stick_ly_f;
		float stick_rx_f;
		float stick_ry_f; 
	bool twist_lock_;
	bool hat_lock_;
        void correctForDeadzone();
        void setMessageValues();
        int GetButtonState (const GAMEPAD_BUTTON button);

};

#endif