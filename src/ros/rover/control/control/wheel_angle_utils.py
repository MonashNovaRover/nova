import math
import numpy as np
import matplotlib.pyplot as plt

CHASSIS_WIDTH = 0.7
CHASSIS_LENGTH = 0.84

angle_offset = math.atan(CHASSIS_WIDTH/CHASSIS_LENGTH)


def calc_wheel_angle(radius, left, direction):
    if left:
        return (0 if radius == float('inf') else -(math.atan((2*radius*direction + CHASSIS_WIDTH)/CHASSIS_LENGTH) - direction*math.pi/2)) + angle_offset
    else:
        return (0 if radius == float('inf') else (math.atan((2*radius*direction - CHASSIS_WIDTH)/CHASSIS_LENGTH) - direction*math.pi/2)) + angle_offset

def radius_from_angle(angle, left):
    if left:
        dir = 1 if angle < -angle_offset else -1
        return float('inf') if angle == angle_offset else (math.tan(-angle + angle_offset + math.pi/2)*CHASSIS_LENGTH - CHASSIS_WIDTH)/(2*dir)
    else:
        dir = 1 if angle < -angle_offset else -1
        return float('inf') if angle == angle_offset else (math.tan(angle - angle_offset + math.pi/2)*CHASSIS_LENGTH + CHASSIS_WIDTH)/(2*dir)




def hex_to_degrees(h):
    #find the twos complement of the hex value
    if h >> 15 == 1:
          h = -((~h + 1) & 0xFFFF)
    #convert to degrees
    return h * 90 / 0x7FFF

def degrees_to_hex(d):
    #convert to hex
    h = d * 0x7FFF / 360
    #find the twos complement of the hex value
    return (~h + 1) & 0xFFFF



if __name__ == "__main__":
    angles = np.arange(-math.pi/2, math.pi/2, 0.001)
    radius = [radius_from_angle(a, True) for a in angles]
    plt.plot(angles, radius)
    #plt.plot(radius, right_angles, label="right")
    plt.legend()
    plt.show()
