# A reading from the NIR probe

# The latest reading from the NIR probe
# Units are arbitrary, and need to be applied to a calibration curve to get meaningful data.
int32[] data


# Status of the NIR Probe
# False = Off, True = Currently taking reading
bool status

# False = No reading has been taken, True = A reading has been taken
bool reading_taken