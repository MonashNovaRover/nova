/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

PACKAGE: 	control
AUTHOR(S):	Matthew Gu
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "unified_arm_input.h"

#include "joystick_translate.h"
#include "keyboard_translate.h"

UnifiedArmInput::UnifiedArmInput(bool joystick_override) { 
    this->joystick_override = joystick_override;
    this->input_devices[JOYSTICK_INDEX] = new JoystickTranslate();
    this->input_devices[KEYBOARD_INDEX] = new KeyboardTranslate();
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

void UnifiedArmInput::joystick_deadline_callback()
{
    deadline_callback(InputDeviceIndex.JOYSTICK_INDEX);
}

void UnifiedArmInput::keyboard_deadline_callback()
{
    deadline_callback(InputDeviceIndex.KEYBOARD_INDEX);
}

void UnifiedArmInput::deadline_callback(int device_index)
{
    input_devices[device_index].reset_message();
}

InputDevice& UnifiedArmInput::get_input_device()
{
    if (joystick_override) {
        return input_devices[JOYSTICK_INDEX]->is_connected() ? *input_devices[JOYSTICK_INDEX] : *input_devices[KEYBOARD_INDEX];
    } else {
        return input_devices[KEYBOARD_INDEX]->is_connected() ? *input_devices[KEYBOARD_INDEX] : *input_devices[JOYSTICK_INDEX];
    }
}

CommonInputCollections::ControlSchemeInputs UnifiedArmInput::get_arm_lock_inputs(){
    return get_input_device().get_arm_lock_inputs();
}

CommonInputCollections::ControlSchemeInputs UnifiedArmInput::get_control_scheme_inputs(){
    return get_input_device().get_control_scheme_inputs();
}

CommonInputCollections::EndEffectorInputs UnifiedArmInput::get_end_effector_inputs(){
    return get_input_device().get_end_effector_inputs();
}

CommonInputCollections::JointVelocityInputs UnifiedArmInput::get_joint_velocity_inputs(){
    return get_input_device().get_joint_velocity_inputs();
}

CommonInputCollections::TwistInputs UnifiedArmInput::get_twist_inputs(){
    return get_input_device().get_twist_inputs();
}
