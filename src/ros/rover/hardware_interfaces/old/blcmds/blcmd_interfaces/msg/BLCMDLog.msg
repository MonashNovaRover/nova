# a log message from a blcmd (see constants.h in blcmd firmware)

# blcmd id
uint8 id

# type or severity of this log (see constants below)
uint8 type

uint8 ERROR = 0
uint8 WARNING = 1
uint8 INFO = 2
uint8 GATE_DRIVER_CONDITION = 3

# log message (usually are ids)
uint8[] message