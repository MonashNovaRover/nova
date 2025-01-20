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

# Status of the NIR Probe
# 0 = Off, 1 = Currently taking LED Water reading, 2 = Currently taking LED Ice reading
bool status
