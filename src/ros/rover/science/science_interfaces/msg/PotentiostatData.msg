# Data from the Potentiostat sensor
# Published on /science/potentiostat/data

std_msgs/Header header

# Channel being measured (0 or 1)
uint8 channel

# Voltage reading in Volts (V)
float32 voltage

# Current reading in milliamps (mA)
float32 current

# True while actively receiving data, False when measurement complete
bool is_receiving
