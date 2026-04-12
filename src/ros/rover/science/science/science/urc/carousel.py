#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the URC UV Vis Spec Carousel

Is position controlled.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: CarouselController
TOPICS:
  - publisher: /science/carousel/position       [NamedPositions] (0-indexed)
SERVICES:
	- service: /science/carousel/set_position   [SetNamedPositions] (0-indexed)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - inner_ring/position         (in degrees)
  - outer_ring/position         (in degrees)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       12/04/26
EDITED:         12/04/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from enum import Enum
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface
from python_control2.hardware_interfaces import PositionalServoHardware
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSDurabilityPolicy
from science_interfaces.msg import NamedPositions
from science_interfaces.srv import SetNamedPositions, SetNamedPositions_Request, SetNamedPositions_Response

class CarouselController(Controller):
    # Ring configs
    class RING(Enum):
        INNER = 0
        OUTER = 1

    RING_NAMES = [
        "inner",
        "outer"
    ]

    # Command interfaces
    ring_cmds: list[Interface]

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        # Set up params
        self.zero_offset = [0, 0]
        self.target_positions = self.declare_parameter("initial_positions", [0,0], "Initial position to set each of the rings").value
        self.zero_cuvettes = self.declare_parameter("zero_cuvettes", [0, 0], "The cuvettes at the spec in the zero position").value
        self.num_cuvettes = self.declare_parameter("num_cuvettes", [15, 24], "The number of cuvettes in each ring").value
        self.max_rotation = self.declare_parameter("max_rotation", [360, 360], "The max rotation in each ring").value

        # Set up publisher
        # Keep last message in the topic for any new subscribers (can publish fewer messages)
        qos_profile = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL
        )
        self.publisher = self.node.create_publisher(NamedPositions, "science/carousel/position", qos_profile)

        # Set up service
        self.set_position_service = self.node.create_service(SetNamedPositions, "science/carousel/set_position", self.set_position_callback)


    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Save references to interfaces
        self.ring_cmds = [command_interfaces[f"{x}/position"] for x in self.RING_NAMES]
        for i in range(len(self.ring_cmds)):
            self.ring_cmds[i].value = self.target_positions[i]
        self.publish()

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        for i in range(len(self.ring_cmds)):
            self.ring_cmds[i].value = self.target_positions[i]

    def publish(self):
        msg = NamedPositions()

        msg.names = []
        msg.positions = []

        for i in range(len(self.RING_NAMES)):
            # Publish current degrees
            msg.names.append(f"{self.RING_NAMES[i]}_degree")
            msg.positions.append(self.get_degrees(self.RING(i)))

            # Publish current cuvette
            msg.names.append(f"{self.RING_NAMES[i]}_cuvette")
            msg.positions.append(float(self.to_cuvette(self.RING(i), self.get_degrees(self.RING(i)))))

        self.publisher.publish(msg)

    def set_position_callback(self, request: SetNamedPositions_Request, response: SetNamedPositions_Response):
        names: list[str] = request.names
        positions: list[float] = request.positions

        # Do validation checks
        if len(names) != len(positions):
            self.logger.error(f"set_position request has mismatched names and position lengths: {names}: {positions}")
            response.success = False
            return response
        if len(names) == 0:
            self.logger.error(f"set_position request 0 elements")
            response.success = False
            return response

        # Evaluate each set position request
        for i in range(len(names)):
            ring_name, action = names[i].split("_")
            try:
                ring = self.RING(self.RING_NAMES.index(ring_name))
            except ValueError:
                self.logger.error(f"Unknown ring: {ring_name}")
                response.success = False
                return response

            if action == "degree":
                response.success = self.set_degrees(ring, positions[i])
                self.logger.info(f"Successfully moved {ring.name} to {positions[i]}°")
            elif action == "cuvette":
                response.success = self.set_degrees(ring, self.to_degrees(ring, round(positions[i])))
                self.logger.info(f"Successfully moved {ring.name} to cuvette {positions[i]}")
            elif action == "zero":
                response.success = self.update_zero(ring, positions[i])
            else:
                self.logger.error(f"Unknown action: {action} - action must be one of: `degree`, `cuvette` or `zero`")
                response.success = False
                return response

        self.publish()
        return response

    def update_zero(self, ring: RING, offset: float) -> bool:
        """ Updates the zero offset """
        self.zero_offset[ring.value] += offset
        self.logger.info(f"Updated zero offset for {ring.name} ring by {offset} to {self.zero_offset[ring.value]}")
        return True

    def get_degrees(self, ring: RING) -> float:
        """ Returns the current position of the ring with the zero offset applied """
        return self.clamp_position(ring, self.ring_cmds[ring.value].value + self.zero_offset[ring.value])

    def set_degrees(self, ring: RING, degrees: float) -> bool:
        """ Set current degrees and apply the offset """
        if degrees < 0 or degrees > self.max_rotation[ring.value]:
            self.logger.error(f"{degrees} is out of bounds [0,{self.max_rotation[ring.value]}]")
            return False

        self.target_positions[ring.value] = self.clamp_position(ring, degrees - self.zero_offset[ring.value])
        return True

    def clamp_position(self, ring: RING, pos: float) -> float:
        """ Ensures the position is between 0 and max position """
        return pos % self.max_rotation[ring.value]

    def to_cuvette(self, ring: RING, degrees: float) -> int:
        """ Returns the closest cuvette at that degree """
        step = self.max_rotation[ring.value] / self.num_cuvettes[ring.value]
        return round(degrees / step) % self.num_cuvettes[ring.value]

    def to_degrees(self, ring: RING, cuvette: int) -> float:
        """ Returns the degrees corresponding to the cuvette
        assumes cuvette is between [0 and max)
        """
        step = self.max_rotation[ring.value] / self.num_cuvettes[ring.value]
        return cuvette * step


if __name__ == "__main__":
    rclpy.init()
    node = Node("carousel")

    # URC 2026 Carousel system
    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", CarouselController) \
        .with_hardware("inner", PositionalServoHardware, can_id=0x0E5, angular_limit=360) \
        .with_hardware("outer", PositionalServoHardware, can_id=0x0E6, angular_limit=360) \
        .with_jcan() \
        .spin()