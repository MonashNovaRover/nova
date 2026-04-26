#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Control for the URC Litmus Strip Dipper
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: litmus_dipper
TOPICS:
  - publisher: /science/litmus_dipper/status   [PumpStatus]
SERVICES:
  - service: /science/litmus_dipper/dip        [RunPump]
  - service: /science/litmus_dipper/stop       [Trigger]
  - service: /science/litmus_dipper/twitch     [SetPosition]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - servo/position            [in degrees between 0 and 180]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):  Binuda Kalugalage
CREATION:   25/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Contexts
from python_control2.controllers import PresetTwitchController
from python_control2.hardware_interfaces import PositionalServoHardware
from science_interfaces.msg import PumpStatus
from science_interfaces.srv import RunPump
from std_srvs.srv import Trigger


class LitmusDipperController(PresetTwitchController):

    def __init__(self, contexts: Contexts,
                 dip_service_name: str = "/science/litmus_dipper/dip",
                 stop_service_name: str = "/science/litmus_dipper/stop",
                 status_topic_name: str = "/science/litmus_dipper/status",
                 **kwargs):
        super().__init__(contexts, **kwargs)

        # Dip cycle state
        self.is_dipping = False
        self.dip_start_time = 0.0
        self.dip_duration = 0.0

        # Services
        self.dip_service = self.node.create_service(RunPump, dip_service_name, self.dip_callback)
        self.stop_service = self.node.create_service(Trigger, stop_service_name, self.stop_callback)

        # Status publisher
        self.status_publisher = self.node.create_publisher(PumpStatus, status_topic_name, 10)

    def end_dip(self):
        """End the dip cycle and return to raised position"""
        self.is_dipping = False
        self.set_preset("raised")
        self.publish_status()

    def dip_callback(self, request: RunPump.Request, response: RunPump.Response) -> RunPump.Response:
        """
        Handle run pump service requests. Non-blocking - returns immediately.

        :param request: The service request with pump hardware name and duration
        :param response: The service response
        :returns: Response indicating success/failure
        """
        if self.is_dipping:
            response.success = False
            response.message = "Already dipping"
            return response
        if request.duration <= 0:
            response.success = False
            response.message = "Invalid duration"
            return response

        self.dip_duration = request.duration
        self.dip_start_time = self.node.get_clock().now().nanoseconds * 1e-9
        self.is_dipping = True
        self.set_preset("lowered")

        response.success = True
        response.message = f"Dipping for {request.duration:.2f}s"
        return response
    
    def stop_callback(self, request: RunPump.Request, response: RunPump.Response) -> RunPump.Response:
        self.end_dip()
        response.success, response.message = True, "Stopped"
        return response

    def on_update(self, now, period):
        super().on_update(now, period)
        if self.is_dipping:
            if now - self.dip_start_time >= self.dip_duration:
                self.end_dip()
            else:
                self.publish_status()

    def publish_status(self):
        msg = PumpStatus()
        msg.running = self.is_dipping
        msg.pump = "litmus_dipper"
        if self.is_dipping:
            msg.time_elapsed = float(self.node.get_clock().now().nanoseconds * 1e-9 - self.dip_start_time)
            msg.time_target = float(self.dip_duration)
        self.status_publisher.publish(msg)

if __name__ == "__main__":
    rclpy.init()
    node = Node("litmus_dipper")

    PythonControl(node, update_rate=10, can_bus="can1") \
    .with_controller("controller", 
                    LitmusDipperController,
                    twitch_service="/science/litmus_dipper/twitch",
                    positions={
                        "raised": 0.0, 
                        "lowered": 180.0
                    }) \
        .with_hardware("dipper", PositionalServoHardware, can_id=0x0E0) \
        .with_jcan() \
        .spin()