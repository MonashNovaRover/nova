#value between 0 and 1 for the speed multiplier
float32 multiplier

#bool for locked
bool locked

#bool for autonomous_mode
bool autonomous_mode

#bool for gamepad connected
bool connected

#enum for drive mode
byte PIVOT = 1
byte STRAFE = 2
byte TANK = 3

#current drive mode of the rover
byte drive_mode

#handbrake on or off
bool handbrake