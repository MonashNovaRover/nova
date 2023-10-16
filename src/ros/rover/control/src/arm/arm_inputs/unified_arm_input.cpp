#include "unified_arm_input.h"

#include "print/print.h"

UnifiedArmInput::UnifiedArmInput(bool joystick_override) { 
    this->joystick_override = joystick_override;
    this->input_devices = {JoystickTranslate(), KeyboardTranslate()};
}

void UnifiedArmInput::joystick_l_callback(const core::msg::InputJoystick::SharedPtr msg)
{
    joystick_l = *msg;
    input_devices[InputDeviceIndex.JOYSTICK_INDEX].set_message(msg, 0);
}

void UnifiedArmInput::joystick_r_callback(const core::msg::InputJoystick::SharedPtr msg)
{
    joystick_r = *msg;
    input_devices[InputDeviceIndex.JOYSTICK_INDEX].set_message(msg, 0);
}

void UnifiedArmInput::keyboard_callback(const core::msg::InputKeyboard::SharedPtr msg)
{
    keyboard = *msg;
    input_devices[InputDeviceIndex.KEYBOARD_INDEX].set_message(msg, 0);
}

InputDevice& UnifiedArmInput::get_input_device()
{
    if (joystick_override) {
        if (joystick_l.connected && joystick_r.connected) {
            return input_devices[InputDeviceIndex.JOYSTICK_INDEX];
        } else {
            return input_devices[InputDeviceIndex.KEYBOARD_INDEX];
        }
    } else {
        if (keyboard.connected) {
            return input_devices[InputDeviceIndex.KEYBOARD_INDEX];
        } else if (joystick_l.connected && joystick_r.connected) {
            return input_devices[InputDeviceIndex.JOYSTICK_INDEX];
        }
    }
}
