#!/usr/bin/env python3
# Purpose: Autonomous typing

import rclpy

class EndEffectorActionServer():

    # OLD CAN commands:
    """
    end_effector = new CMD(bus=1, id=7, PWM=3, 1);

    velocity is a float between -1 and 1
    FOR DRIVING END EFFECTOR
    frame.id = (id << 4) | drive_mode;
    frame.len = 2;

    // Scale the speed to the range
    int16_t scaled_velocity = convert_to_int16(velocity);

    // Order data in big-endian order (MSB first)
    frame.data[0] = scaled_velocity >> 8;
    frame.data[1] = scaled_velocity & 0xFF;

    FOR SETTING LINEAR ACTUATOR OF END EFFECTOR
    unsigned char actuation = 0;
    if (value < 0){
        actuation = 1;
    }
    else if (value > 0){
        actuation = 2;
    }

    // Create a new CAN frame
    scpp::CanFrame frame;
    frame.id = (id << 4) | CMDCommand::ACTUATOR=7;
    frame.len = 2;

    // Order data in big-endian order (MSB first)
    frame.data[0] = actuation;

    // Write the frame to the bus
    can_socket.write(frame);
    """

    def __init__(self):
        pass


def main():
    rclpy.init()
    node = EndEffectorActionServer()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()