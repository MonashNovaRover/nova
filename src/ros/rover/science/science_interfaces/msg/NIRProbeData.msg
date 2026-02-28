# A reading from the NIR probe

# The latest reading from the NIR probe
# Units are arbitrary, and need to be applied to a calibration curve to get meaningful data.
int16[] data


# Status of the NIR Probe
# 0 = Off, 1 = Currently taking LED Water reading, 2 = Currently taking LED Ice reading
byte status