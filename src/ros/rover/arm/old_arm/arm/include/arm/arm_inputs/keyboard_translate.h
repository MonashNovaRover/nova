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
EDITED:		18/12/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
Ask operator for key mapping, specifically the frame controls
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#include "arm_inputs/input_device.h"
#include "input_interfaces/msg/input_keyboard.hpp"

#include <string>
#include <map>
#include <unordered_set>

class KeyboardTranslate: public InputDevice {
    //------------------------------------------------------------//
    private:

    /// @brief  The keyboard message
    input_interfaces::msg::InputKeyboard keyboard;

    /// @brief Buffer for messages to print
    std::string message = "";

    /// @brief  speeds increment each time it is increased or decreased. 
    ///         This is to allow for gradual speed changes. 
    ///         Currently allows about 10 levels. 
    const float speed_increment = 0.1f;

    /// @brief  The actual current speed for joints
    float speed = 0.3f;

    /// @brief  The speed to display in 0 to 1 scale
    float display_speed;

    /// @brief  Base frame offset
    int8_t base_frame_offset;
    
    /// @brief  Sets to false whenever a callback is received, to not over sample key presses
    ///         Oddly enough, we sample inputs every 50ms but updates/publishes every 10ms
    ///         I am not sure why this is the case, but I will keep it. 
    bool updated_controls;

    /// @brief  Sets to false whenever a callback is received, to not over sample key presses
    bool updated_speed;

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

    /// @brief The key pressed map for the keyboard for pre-processing
    std::unordered_set<uint32_t> key_pressed_set;

    /// @brief The key held map for the keyboard for pre-processing
    std::unordered_set<uint32_t> key_held_set;

    /// @brief The key mappings for the keyboard
    typedef struct {
        // Control scheme
        uint32_t input_lock;
        uint32_t joint_limits;

        uint32_t ik_linear;
        uint32_t ik_angular;
        uint32_t flat_frame_linear;
        uint32_t flat_frame_angular;
        uint32_t endpoint_frame_linear;
        uint32_t endpoint_frame_angular;
        uint32_t use_spm_roll;
        uint32_t position_control;
        uint32_t all_joint_space;
        uint32_t all_task_space;
        uint32_t zero_resolvers;
        uint32_t toggle_input;
        std::map<int, uint32_t> resolver_to_key;
        
        // Shift based
        uint32_t speed_increase;
        uint32_t speed_decrease;
        uint32_t base_frame_offset_toggle;

        // End effector
        uint32_t end_effector_actuation_increase;
        uint32_t end_effector_actuation_decrease;
        uint32_t linear_actuation_increase;
        uint32_t linear_actuation_decrease;
        // uint32_t laser;
        // uint32_t hex_key_increase;
        // uint32_t hex_key_decrease;
        // uint32_t finger_increase;
        // uint32_t finger_decrease;

        // Joint Space control
        uint32_t joint_1_increase;
        uint32_t joint_1_decrease;
        uint32_t joint_2_increase;
        uint32_t joint_2_decrease;
        uint32_t joint_3_increase;
        uint32_t joint_3_decrease;
        uint32_t joint_4_increase;
        uint32_t joint_4_decrease;
        uint32_t joint_5_increase;
        uint32_t joint_5_decrease;
        uint32_t joint_6_increase;
        uint32_t joint_6_decrease;
        
        // Task Space control
        uint32_t x_increase;
        uint32_t x_decrease;
        uint32_t y_increase;
        uint32_t y_decrease;
        uint32_t z_increase;
        uint32_t z_decrease;
        uint32_t roll_increase;
        uint32_t roll_decrease;
        uint32_t pitch_increase;
        uint32_t pitch_decrease;
        uint32_t yaw_increase;
        uint32_t yaw_decrease;
    } KeyMappings;
    KeyMappings key_mappings;

    /// @brief  Sets the key mappings
    void set_key_mappings();

    /// @brief  Preprocesses the keys pressed or held
    void read_keys();

    /// @brief  Searches for if a key is pressed
    /// @param  key SDL Scancode of the key, OR'd with masks
    /// @return Returns true if the key is pressed
    bool is_pressed(uint32_t key);

    /// @brief  Searches for if a key is held
    /// @param  key SDL Scancode of the key, OR'd with masks
    /// @return Returns true if the key is held
    bool is_held(uint32_t key);

    /// @brief  Searches for if a key is pressed or held
    /// @param  key SDL Scancode of the key, OR'd with masks
    /// @return Returns true if the key is pressed or held
    bool is_pressed_or_held(uint32_t key);

    /// @brief Toggles the given control input, and sets the update flag
    /// @param field_name The name of the field to toggle
    /// @param value The value of the field to toggle
    /// @param key The key to toggle the field with
    void toggle_control(std::string field_name, bool& value, uint32_t key);

    // Function below allows separation of ctrl+key and key

    /// @brief  Ctrl masks a key
    /// @param  key SDL Scancode of the key
    /// @return Returns the key with control masked
    uint32_t ctrl(uint32_t key);

    /// @brief  Shift masks a key
    /// @param  key SDL Scancode of the key
    /// @return Returns the key with shift masked
    uint32_t shift(uint32_t key);

    /// @brief  Alt masks a key
    /// @param  key SDL Scancode of the key
    /// @return Returns the key with alt masked
    uint32_t alt(uint32_t key);

    /// @brief  changes the speed of the arm (EE, joints or twist)
    void change_speed();

    //------------------------------------------------------------//
    public:

    /// @brief  Constructor that does nothing
    KeyboardTranslate();

    // See input_device.h for documentation
    bool get_control_scheme_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs) override;

    void get_end_effector_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, arm_interfaces::msg::EndEffectorInput& end_effector_inputs) override;

    void get_joint_velocity_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, sensor_msgs::msg::JointState& joint_velocity_inputs) override;
    
    void get_twist_inputs(arm_interfaces::msg::ArmControlScheme& control_scheme_inputs, geometry_msgs::msg::TwistStamped& twist_inputs) override;

    /// @brief Callback for keyboard messages
    /// @param msg The keyboard message
    void keyboard_callback(input_interfaces::msg::InputKeyboard::SharedPtr msg);

    void reset_message() override;

    bool is_connected() override;
};
