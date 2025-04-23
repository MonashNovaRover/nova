# A reading from the NIR probe

# The latest reading from the NIR probe
# Units are arbitrary, and need to be applied to a calibration curve to get meaningful data.
int32 data

# enum for the current cache
byte CACHE_OPEN = 0
byte CACHE_1_CLOSED = 1
byte CACHE_2_CLOSED = 2
byte CACHE_CLOSED = 3

