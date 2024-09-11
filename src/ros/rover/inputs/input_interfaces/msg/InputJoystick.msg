# Joystick Input Message
bool connected


# Button State Data
# State input flags show the button frame state where:
#   0 - Nothing
#   1 - Down
#   2 - Held
#   3 - Up

# Buttons located near the thumb stick
int8 btn_thumb_l_state      # Left
int8 btn_thumb_r_state      # Right
int8 btn_thumb_u_state      # Up (behind the thumbstick)
int8 btn_thumb_d_state      # Down

# Buttons located at the bottom left
# Left to Right, First then Second row
int8 btn_bottom_l1_state
int8 btn_bottom_l2_state
int8 btn_bottom_l3_state
int8 btn_bottom_l4_state
int8 btn_bottom_l5_state
int8 btn_bottom_l6_state

# Buttons located at the bottom right
# Left to Right, First then Second row
int8 btn_bottom_r1_state
int8 btn_bottom_r2_state
int8 btn_bottom_r3_state
int8 btn_bottom_r4_state
int8 btn_bottom_r5_state
int8 btn_bottom_r6_state


# Main Joystick Axis Data
# Each axis data ranges between -1 and 1
# By default, no input from an axis will be 0
float32 ax_stick_x
float32 ax_stick_y
float32 ax_stick_twist

# Thumb Axis Data
# Each Thumb axis can be -1, 0 or 1
float32 ax_thumb_x
float32 ax_thumb_y

# Slider Data
# This ranges from 0 to 1
float32 ax_slider