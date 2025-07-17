# Sends data for the distance sensor (fake data)

# Gets a random value for the integer
randA=$((0 + $RANDOM % 2))
randB=$((0 + $RANDOM % 10))
randC=$((0 + $RANDOM % 10))

# Send the CAn value
cansend can1 004#0$randA.$randB$randC
