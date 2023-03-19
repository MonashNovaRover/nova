# Displays kilns and bilns data.
# ex. 4A1#01 8 hex digits signed - 2's complement. (Big Kiln)
# ex. 4B1#0(1/2/3)8 hex digits signed. (Baby Kilns)

#
# Kiln Id
int8 id

# Kiln Mass [g]
float32 mass

# Kiln time interval since start [s]
int8 interval