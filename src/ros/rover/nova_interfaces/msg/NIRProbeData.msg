# A reading from the NIR probe

# The latest reading from the NIR probe
# Units are arbitrary, and need to be applied to a calibration curve to get meaningful data.
int32 data

# enum for the current led
byte LED_WATER = 1
byte LED_ICE = 2
byte LED_OFF = 3


# The LED (if any) used to take the reading
byte led

# Calibration period before turning light on
bool calibrating
