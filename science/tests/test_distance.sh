# Sends data for the distance sensor (fake data)

# Gets a random value for the integer
randA=$((1 + $RANDOM % 10))
randB=$((1 + $RANDOM % 10))

# Send the CAn value
cansend can1 004#00.$randA$randB