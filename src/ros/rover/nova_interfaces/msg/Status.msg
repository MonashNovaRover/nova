# The current status of the rover autonomous stack
int8 status

# enum for the current status
byte IDLE = 0
byte TRAVERSING = 1
byte SEARCHING = 2
byte ARRIVED_SUCCESSFULLY = 3
byte ARRIVED_UNSUCCESSFULLY = 4
