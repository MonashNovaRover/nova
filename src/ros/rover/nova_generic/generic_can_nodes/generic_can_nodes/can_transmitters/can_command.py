#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node service that takes in an integer command
and sends a corresponding CAN messages on the bus.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        generic_can_nodes
AUTHOR:         Felicity Matthews
CREATION:	    24/07/2025
EDITED:		    24/07/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
to run node with cache example config:
ros2 run generic_can_nodes can_command.py --ros-args --params-file /home/nova/nova/src/ros/rover/nova_generic/generic_can_nodes/generic_can_nodes/can_transmitters/config/example.yaml
"""

import rclpy
import jcan, logging
from rclpy.node import Node

from generic_can_nodes.can_command_parameters import can_command_parameters
from generic_can_nodes.can_str_msg_parser import parse
from generic_interfaces.srv import Int32

class CANCommand(Node):
    """Class to represent a CAN Transmitter that takes commands and sends corresponding CAN messages """

    def __init__(self):
        super().__init__("can_command")

        # declare parameters
        self.param_listener = can_command_parameters.ParamListener(self)
        self.params = self.param_listener.get_params()

        # create command -> [name, CAN frame] map
        self.map: dict[int, tuple[str, jcan.Frame]] = dict()
        self.populate_map()

        # create service
        print(self.get_name())
        print("params:", self.params, " service:", self.params.service, "int array:", self.params.commands)
        self.create_service(Int32, self.params.service, self.service_callback)

        # start can
        self.bus = jcan.Bus()
        self.bus.open(self.params.can_bus)

        # update logger and signal successful start
        self.get_logger().set_level(logging.getLevelNamesMapping()[self.params.logging_level])
        self.get_logger().info(f"{self.get_name()} started with: {self.map}")

    def populate_map(self):
        """ populates the map with command: (name, frame) """
        self.map.clear()
        assert len(self.params.commands) == len(self.params.command_names) == len(self.params.can_messages), "CAN commands, names and message lengths do not match"

        for i in range(len(self.params.commands)):
            command = self.params.commands[i]
            assert command not in self.map, f"Duplicate commands: {command}"
            self.map[command] = (self.params.command_names[i], parse(self.params.can_messages[i]))

    def service_callback(self, request: Int32.Request, response: Int32.Response):
        """ service callback that sends the corresponding CAN message to the request """
        name, frame = self.map[request.data]
        self.get_logger().info(f"Received command {name} [{request.data}] -> sending {str(frame)}")

        try:
            self.get_logger().debug(f"Sending {frame}")
            self.bus.send(frame)
            response.success = True
        except Exception as e:
            self.get_logger().error(f"Failed to send CAN message: {e}")
            response.success = False

        return response

def main():
    rclpy.init()
    node = CANCommand()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
