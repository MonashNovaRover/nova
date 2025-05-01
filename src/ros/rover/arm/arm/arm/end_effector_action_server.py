#!/usr/bin/env python3
# Purpose: Autonomous typing

import rclpy

class EndEffectorActionServer():

    # CAN commands when found will go here

    def __init__(self):
        pass


def main():
    rclpy.init()
    node = EndEffectorActionServer()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()