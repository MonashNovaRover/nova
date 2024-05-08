# Gamepad Input Message
bool connected


# Button State Data
# State input flags show the button frame state where:
#   0 - Nothing
#   1 - Down
#   2 - Held
#   3 - Up
int8 btn_a_state
int8 btn_b_state
int8 btn_x_state
int8 btn_y_state
int8 btn_shoulder_l_state
int8 btn_shoulder_r_state
int8 btn_back_state
int8 btn_start_state
int8 btn_xbox_state
int8 btn_thumb_l_state
int8 btn_thumb_r_state
int8 btn_dpad_l_state
int8 btn_dpad_r_state
int8 btn_dpad_u_state
int8 btn_dpad_d_state


# Axis Data
# Each axis data ranges between -1 and 1
# By default, no input from an axis will be 0
float32 ax_stick_l_x
float32 ax_stick_l_y
float32 ax_stick_r_x
float32 ax_stick_r_y


# DPAD Data
# Each DPAD axis can be -1, 0 or 1
float32 ax_dpad_x
float32 ax_dpad_y


# Trigger Data
# Each trigger ranges between 0 and 1
float32 trg_l_val
float32 trg_r_val

# Each trigger also has a state of whether it is pressed
bool    btn_trigger_l_down
bool    btn_trigger_r_down