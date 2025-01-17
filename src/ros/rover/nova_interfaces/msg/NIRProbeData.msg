# A reading from the NIR probe

# The latest reading from the NIR probe
# Units are arbitrary, and need to be applied to a calibration curve to get meaningful data.
float32 data
float32 std_dev

# enum for the current photodiode
byte PHOTODIODE_OFF = 0
byte PHOTODIODE_WATER = 1
byte PHOTODIODE_ICE = 2

# The photodiode (if any) used to take the reading
# 0 = off | 1 = Photodiode 1 | 2 = Photodiode 2
byte active_photodiode

# Determine on GUI if LED is on
bool led_on
