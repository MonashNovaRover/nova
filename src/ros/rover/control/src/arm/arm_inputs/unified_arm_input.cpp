#include "unified_arm_input.h"

#include "print/print.h"

UnifiedArmInput::UnifiedArmInput(bool joystick_override) { 
    this->joystick_override = joystick_override;
    this->input_devices = {JoystickTranslate(), KeyboardTranslate()};
}

void UnifiedArmInput::joystick_l_callback(const core::msg::InputJoystick::SharedPtr msg)
{
    input_devices[InputDeviceIndex.JOYSTICK_INDEX].set_message(msg, 0);
}

void UnifiedArmInput::joystick_r_callback(const core::msg::InputJoystick::SharedPtr msg)
{
    input_devices[InputDeviceIndex.JOYSTICK_INDEX].set_message(msg, 1);
}

void UnifiedArmInput::keyboard_callback(const core::msg::InputKeyboard::SharedPtr msg)
{
    input_devices[InputDeviceIndex.KEYBOARD_INDEX].set_message(msg, 0);
}

void UnifiedArmInput::deadline_callback(int device_index)
{
    input_devices[device_index].reset_message();
}

InputDevice& UnifiedArmInput::get_input_device()
{
    if (joystick_override) {
        if (input_devices[InputDeviceIndex.JOYSTICK_INDEX].is_connected()) {
            return input_devices[InputDeviceIndex.JOYSTICK_INDEX];
        } else {
            return input_devices[InputDeviceIndex.KEYBOARD_INDEX];
        }
    } else {
        if (input_devices[InputDeviceIndex.KEYBOARD_INDEX].is_connected()) {
            return input_devices[InputDeviceIndex.KEYBOARD_INDEX];
        } else if (joystick_l.connected && joystick_r.connected) {
            return input_devices[InputDeviceIndex.JOYSTICK_INDEX];
        }
    }
}
