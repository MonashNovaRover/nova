# Navigation Satellite fix for any Global Navigation Satellite System
#
# Based off the sensor_msgs/msg/NavSatFix message
#
# Specified using the WGS 84 reference ellipsoid
# header.stamp specifies the ROS time for this measurement (the
# corresponding satellite time may be reported using the
# sensor_msgs/TimeReference message).
#
# header.frame_id is the frame of reference reported by the satellite
# receiver, usually the location of the antenna. This is a
# Euclidean frame relative to the vehicle, not a reference
# ellipsoid.
std_msgs/Header header

# Satellite fix status information.
sensor_msgs/NavSatStatus status

# Latitude [degrees]. Positive is north of equator; negative is south.
float64 latitude

# Longitude [degrees]. Positive is east of prime meridian; negative is west.
float64 longitude

# Altitude [m]. Positive is above the WGS 84 ellipsoid
# (quiet NaN if no altitude is available).
float64 altitude

# Heading [degrees]. Between 0 and 360, 0 is North. -1 means no heading.
float64 heading -1
