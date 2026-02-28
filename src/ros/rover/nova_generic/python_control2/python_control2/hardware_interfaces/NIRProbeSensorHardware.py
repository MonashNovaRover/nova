#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware interface for the NIR Probe sensors.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STATE INTERFACES:
  - <sensor>/data  [measurement from photodiode]
PACKAGE:        python_control2
AUTHOR(S):      Yahya Muayyiduddin
CREATION:       27/02/26
EDITED:         27/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from collections.abc import Callable
from typing import Any, List, Dict

import jcan
from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts
from teleop_python_utils import Inputs, EventCollection


class NIRProbeSensorHardware(HardwareInterface):
    """
    Hardware interface for NIR probe photodiodes.
    Each sensor has a state interface storing its last measured value.
    """

    def __init__(self, contexts: Contexts,
                 sensors: List[str],
                 can_id: int = 0x4E2,
                 initial_value: Any = 0):
        """
        :param contexts: PythonControl2 contexts
        :param sensors: List of photodiode sensor names
        :param can_id:  CAN ID of hardware
        :param initial_value: Default value before receiving data
        """
        super().__init__(contexts)

        self.sensors = sensors
        self.can_id = can_id
        # self.last_values: Dict[str, Any] = {name: initial_value for name in sensors}

        # CAN bus
        self.bus: jcan.Bus = contexts[jcan.Bus]

        # Setup event to trigger data publishing to gui
        if EventCollection in contexts:
            events = contexts[EventCollection]
            self.publish_reading_event = events.get(f"{self.hardware_name}/publish_reading")
        else:
            self.logger.error("Could not find EventCollection in the python control contexts, cannot zero StepperHardware.")



    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> bool:
        """
        Configure state interfaces and register CAN callbacks.
        """
        self.sensor_states = [state_interfaces[f"{x}/data"] for x in self.sensors]

        self.bus.add_callback(self.can_id, self.frame_callback)
        return True

    def frame_callback(self, frame: jcan.Frame):
        """
        Callback invoked when a CAN frame for sensor is received.
        """
        values: list[int] = self.interpret_data(frame.data)
        for i in range(len(self.sensor_states)):
            self.sensor_states[i].value = values[i]  

        self.logger.debug(f"NIRProbeSensorHardware received CAN message: {frame}")
        self.publish_reading_event.invoke()

    def on_read(self, now: float, period: float):
        """ Called to read values from hardware, and put them into stored state interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    def on_write(self, now: float, period: float):
        """ Called to write to hardware using values stored in command interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    def interpret_data(self, data: bytes) -> list[int]:

        """
        Convert raw CAN bytes into NIR photodiode measurements.

        Expected data layout (8 bytes):
        byte 0-1: PD1_LEDoff (uint16_t, little-endian)
        byte 2-3: PD2_LEDoff (uint16_t, little-endian)
        byte 4-5: PD1_LEDon  (uint16_t, little-endian)
        byte 6-7: PD2_LEDon  (uint16_t, little-endian)
        """
        if len(data) < 8:
            raise ValueError(f"Expected 8 bytes, got {len(data)}")
        PD1_LEDoff = int.from_bytes(data[0:2], "little")
        PD2_LEDoff = int.from_bytes(data[2:4], "little")
        PD1_LEDon =  int.from_bytes(data[4:6], "little")
        PD2_LEDon = int.from_bytes(data[6:8], "little")

        PD1_diff = max(PD1_LEDon - PD1_LEDoff, 0)
        PD2_diff = max(PD2_LEDon - PD2_LEDoff, 0)

        return [PD1_diff, PD2_diff]