# A reading from the NIR probe

# The latest reading from the NIR probe
# Units are arbitrary, and need to be applied to a calibration curve to get meaningful data.
float32 data
float32 std_dev

# enum for the current led
byte LED_OFF = 0
byte LED_WATER = 1
byte LED_ICE = 2

# The LED (if any) used to take the reading
byte led

# Determine on GUI if LED is on
bool led_on

# Determine on GUI if photodiode is on
bool photodiode_on
