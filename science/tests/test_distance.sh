# Sends data for the distance sensor (fake data)

# Gets a random value for the integer
randA=$((0 + $RANDOM % 10))
randB=$((0 + $RANDOM % 10))

# Send the CAn value
cansend can1 004#00.$randA$randB
