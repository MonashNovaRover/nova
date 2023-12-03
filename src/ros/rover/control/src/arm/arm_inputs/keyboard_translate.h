#pragma once
/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class is responsible for translating keyboard
messages into common arm input messages.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Matthew Gu
CREATION:	23/09/2023
EDITED:		02/12/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
Ask operator for key mapping, specifically the frame controls
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "input_device.h"
#include "common_input_collections.h"
#include "core/msg/input_keyboard.hpp"

#include <memory>

class KeyboardTranslate: public InputDevice {
    //------------------------------------------------------------//
    private:

    // below constants need to match the key from inputs/keyboard.h
    /// @brief  The key mask for the control key
    const static int CTRL_MASK = 1<<31;

    /// @brief  The key mask for the shift key
    const static int SHIFT_MASK = 1<<30;

    /// @brief The key mask for the alt key
    const static int ALT_MASK = 1<<29;

    /// @brief  The keyboard message
    core::msg::InputKeyboard keyboard;

    /// @brief  speeds increment each time it is increased or decreased. 
    ///         This is to allow for gradual speed changes. 
    ///         Currently allows about 10 levels. 
    const float speed_increment = 0.01f;
    /// @brief  The current speed for joints
    float joint_twist_speed;
    /// @brief  The current speed for the end effector
    float end_effector_speed;
    /// @brief  Base frame offset
    int8_t base_frame_offset

    /// @brief  The speed multipliers for each set of inputs, copied from Joysticks
    typedef struct {
        // Multiplier for all inputs
        // Tune this to adjust the max velocity of all joints
        float all_inputs = 0.30;
        // Separate multipliers for each set of inputs
        // Tune these so joints move at reasonable speeds relative to each other
        float wrist_joints = 1.20;
        float ik_linear = 0.50;
        float ik_angular = 0.85;
    } SpeedMultipliers;
    SpeedMultipliers speed_multipliers;

    /// @brief  Input collections for control scheme
    CommonInputCollections::ControlSchemeInputs control_scheme_inputs;
    
    /// @brief  Input collections for end effector
    CommonInputCollections::EndEffectorInputs end_effector_inputs;

    /// @brief  Input collections for joint velocity
    CommonInputCollections::JointVelocityInputs joint_velocity_inputs;

    /// @brief  Input collections for twist
    CommonInputCollections::TwistInputs twist_inputs;

    /// @brief  Searches for if a key is pressed
    /// @param  key SDL Scancode of the key, OR'd with masks
    /// @return Returns true if the key is pressed
    bool is_pressed(int key);

    /// @brief  Searches for if a key is held
    /// @param  key SDL Scancode of the key, OR'd with masks
    /// @return Returns true if the key is held
    bool is_held(int key);

    /// @brief  Searches for if a key is pressed or held
    /// @param  key SDL Scancode of the key, OR'd with masks
    /// @return Returns true if the key is pressed or held
    bool is_pressed_or_held(int key);

    /// @brief Toggles the given control input, and sets the update flag
    /// @param toggle The control message to toggle
    /// @param key Corresponding key to check
    /// @return the toggled output
    bool toggle_control(bool toggle, int key);

    // Function below allows separation of ctrl+key and key

    /// @brief  Ctrl masks a key
    /// @param  key SDL Scancode of the key
    /// @return Returns the key with control masked
    int ctrl(int key);

    /// @brief  Shift masks a key
    /// @param  key SDL Scancode of the key
    /// @return Returns the key with shift masked
    int shift(int key);

    /// @brief  Alt masks a key
    /// @param  key SDL Scancode of the key
    /// @return Returns the key with alt masked
    int alt(int key);

    /// @brief  increases the speed of the arm (EE or joints)
    /// @param  value the current speed
    /// @return Returns the new speed
    float increase_speed(float value);

    /// @brief  decreases the speed of the arm (EE or joints)
    /// @param  value the current speed
    /// @return Returns the new speed
    float decrease_speed(float value);
    //------------------------------------------------------------//
    public:

    /// @brief  Constructor that does nothing
    KeyboardTranslate();

    // See documentation in input_device.h
    CommonInputCollections::ControlSchemeInputs get_arm_lock_inputs() override;

    CommonInputCollections::ControlSchemeInputs get_control_scheme_inputs() override;

    CommonInputCollections::EndEffectorInputs get_end_effector_inputs() override;

    CommonInputCollections::JointVelocityInputs get_joint_velocity_inputs() override;

    CommonInputCollections::TwistInputs get_twist_inputs() override;

    /// @brief Callback for keyboard messages
    /// @param msg The keyboard message
    void keyboard_callback(core::msg::InputKeyboard::SharedPtr msg);

    void reset_message();

    bool is_connected();
};
