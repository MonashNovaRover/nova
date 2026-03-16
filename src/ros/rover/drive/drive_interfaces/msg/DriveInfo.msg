#value between 0 and 1 for the speed multiplier
float32 multiplier

#bool for locked
bool locked

# provide reason for why the controller is currently or was previously locked
byte locked_reason 0

# possible locked reasons
byte START_LOCKED = 1
byte LOCK_BUTTON = 2
byte BLCMD_ERRORS = 3
byte GAME_PAD_DISCONNECTED = 4

#bool for autonomous_mode
bool autonomous_mode

#bool for gamepad connected
bool connected

#enum for drive mode
byte PIVOT = 1
byte STRAFE = 2
byte TANK = 3
byte ACKERMANN = 4

#current drive mode of the rover
byte drive_mode 

#handbrake on or off
bool handbrake

#autolock disabled or not
bool override_autolock false