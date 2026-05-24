#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: LED control for UV/Vis spectrometer
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: science_leds
SERVICES:
    - server: /science/leds/set (SetNamedBool)
PUBLISHERS:
    - /science/leds/status (NamedBools)
EVENTS:
    - vis_spec_central_led/toggle, vis_spec_central_led/on, vis_spec_central_led/off
    - vis_spec_nile_red/toggle, vis_spec_nile_red/on, vis_spec_nile_red/off
    - vis_spec_camera/toggle, vis_spec_camera/on, vis_spec_camera/off
    - vis_spec_nadh/toggle, vis_spec_nadh/on, vis_spec_nadh/off
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):	Angel
CREATION:	20/04/2026
EDITED:		24/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSDurabilityPolicy
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection
from science_interfaces.srv import SetNamedBool
from science_interfaces.msg import NamedBools
from python_control2.hardware_interfaces import ToggleHardware
from teleop_python_utils import EventCollection


class LEDController(Controller):

    def __init__(self, contexts: Contexts, led_list: list[str] = []):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.led_list = led_list
        self.led_events = {}
        self.led_states = {led: False for led in led_list}

        #setup service
        self.set_led_service = self.node.create_service(SetNamedBool,"/science/leds/set", self.led_set_callback)

        #setup status publisher with TRANSIENT_LOCAL QoS for late subscribers
        qos_profile = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL
        )
        self.status_publisher = self.node.create_publisher(NamedBools, "/science/leds/status", qos_profile)

        #publish initial state
        self._publish_status()

        #led events for leds in led list (toggle, on, off)
        if EventCollection in contexts:
            events = contexts[EventCollection]
            for led in led_list:
                self.led_events[f"{led}/toggle"] = events.get(f"{led}/toggle")
                self.led_events[f"{led}/on"] = events.get(f"{led}/on")
                self.led_events[f"{led}/off"] = events.get(f"{led}/off")

            # Turn off all LEDs on startup
            for led in led_list:
                off_event = self.led_events.get(f"{led}/off")
                if off_event is not None:
                    off_event.invoke()
        else:
            self.logger.error("Could not find EventCollection in the python control contexts, cannot control LEDs")


    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        pass

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    def _publish_status(self):
        """Publish the current LED states."""
        msg = NamedBools()
        msg.names = list(self.led_states.keys())
        msg.values = list(self.led_states.values())
        self.status_publisher.publish(msg)

    def led_set_callback(self, request, response):
        try:
            if request.name not in self.led_list:
                self.logger.error(f"LED {request.name} not in configured LED list")
                response.success = False
                return response

            # Invoke the appropriate event based on the boolean value
            event_key = f"{request.name}/{'on' if request.value else 'off'}"

            if event_key not in self.led_events:
                self.logger.error(f"Event {event_key} not found in led_events")
                response.success = False
                return response

            led_event = self.led_events[event_key]
            if led_event is None:
                self.logger.error(f"Event {event_key} is None")
                response.success = False
                return response

            led_event.invoke()
            self.led_states[request.name] = request.value
            self._publish_status()
            response.success = True
        except Exception as e:
            self.logger.error(f"An error occurred while attempting to set LED: {e}")
            response.success = False

        return response

if __name__ == "__main__":
    rclpy.init()

    node = Node("science_leds")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", LEDController, led_list = ["vis_spec_central_led", "vis_spec_nile_red","vis_spec_camera", "vis_spec_nadh"]) \
        .with_hardware("vis_spec_central_led", ToggleHardware, can_id = 0x0F2, on_command =0x11 , off_command = 0x10) \
        .with_hardware("vis_spec_nile_red", ToggleHardware, can_id = 0x0F2, on_command =0x21 , off_command = 0x20) \
        .with_hardware("vis_spec_camera", ToggleHardware, can_id = 0x0F2, on_command =0x31 , off_command = 0x30) \
        .with_hardware("vis_spec_nadh", ToggleHardware, can_id = 0x0F2, on_command =0x22 , off_command = 0x40) \
        .with_jcan() \
        .with_event_collection() \
        .spin()
 
        
        