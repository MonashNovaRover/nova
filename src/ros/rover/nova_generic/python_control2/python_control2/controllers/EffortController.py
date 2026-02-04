#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for effort-based systems which receive 
effort input from a service.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TOPICS:
    - publisher: <topic_name> [EffortStatus]
SERVICES:
	- service: <service_name> [EffortCommand]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <hardware_name>/effort    [value between 0 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Jonathan Jia, Binuda Kalugalage
CREATION:       31/01/26
EDITED:         04/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from typing import Optional
from ..controller_manager.Interface import Interface, InterfaceCollection
from ..controller_manager.Contexts import Contexts
from .Controller import Controller
from science_interfaces.msg import EffortStatus
from science_interfaces.srv import EffortCommand


class EffortController(Controller):
    # Command interfaces
    effort_cmd: Interface

    def __init__(self, contexts: Contexts, 
                 hardware_name: str = "hardware_name",
                 service_name: str = "",
                 topic_name: str = "",
                 publish_rate: int = 5):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param hardware_name: Name of hardware interface being used.
        :param service_name: Name of service used to receive effort level and send system status.
        :param topic_name: Name of topic which system status is published to.
        :param publish_rate: Frequency at which system status is published.

        """
        super().__init__(contexts)
        self.logger.info(f"EffortController -- I have been __init__ialized")

        self.hardware_name = self.declare_parameter("hardware_name", hardware_name).value
        self.service_name: str = self.declare_parameter("service_name", service_name).value
        self.topic_name: str = self.declare_parameter("topic_name", topic_name).value
        self.publish_rate: int =  self.declare_parameter("publish_rate", publish_rate).value

        self.is_on = False
        self.effort_level = 0

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
        self.logger.info(f"Getting {self.hardware_name}/effort")
        self.effort_cmd = command_interfaces[f"{self.hardware_name}/effort"]

        # Create status publisher
        self.publisher = self.node.create_publisher(EffortStatus, self.topic_name, 5)
        self.publisher_timer = self.node.create_timer(1 / self.publish_rate, self.publish_data)

        # Create effort service
        self.command_service = self.node.create_service(EffortCommand, self.service_name, self.command_callback)

    def publish_data(self):
        """ Publishes status of system """

        data_msg = EffortStatus()
        data_msg.state = self.is_on
        self.publisher.publish(data_msg)

        self.logger.debug(f"EffortController {self.name} published EffortStatus: {data_msg}")

    def command_callback(self, request: EffortCommand.Request, response: EffortCommand.Response):
        """ Uses request to update effort system's settings and send system status

        :param request: Specified settings for effort system control (state, effort_level).
        :param response: Whether or not these settings were applied successfully (applied).
        """
        self.logger.info(f"EffortController {self.name} received EffortCommand request: (state={request.state}, level={request.level:.2f})")

        self.is_on = request.state
        self.effort_level = request.level

        response.applied = True
        return response
    
    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """

        # Update effort
        self.effort_cmd.value = self.is_on * self.effort_level
