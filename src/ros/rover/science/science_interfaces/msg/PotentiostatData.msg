# Data from the Potentiostat sensor
# Published on /science/potentiostat/data

std_msgs/Header header

# Channel being measured (0 or 1)
uint8 channel

# Voltage reading (raw value)
float32 voltage

# Current reading (raw value)
float32 current

# True while actively receiving data, False when measurement complete
bool is_receiving
