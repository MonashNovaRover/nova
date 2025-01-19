# A reading from the NIR probe

# The latest reading from the NIR probe
# Units are arbitrary, and need to be applied to a calibration curve to get meaningful data.
int32 data

# enum for the current led
byte LED_OFF = 0
byte LED_WATER = 1
byte LED_ICE = 2

# The LED (if any) used to take the reading
byte led

# Whether or not the NIR probe is currently taking a reading
bool reading_in_progress
