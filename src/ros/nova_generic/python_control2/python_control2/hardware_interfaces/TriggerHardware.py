"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware that can be triggered to send a single
can command.

For simple applications where no logic or
customization is needed.

Triggered by invoking the "<joint>/trigger" event.
(found in the EventCollection context)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EVENTS:
  - <joint>/trigger
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Felicity Matthews
CREATION:       28/02/26
EDITED:         28/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import jcan

from ..controller_manager.Interface import InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts
from teleop_python_utils import EventCollection


class TriggerHardware(HardwareInterface):
    can_id: int
    can_message: list[int]
    # The name of the joint
    joint: str

    def __init__(self, contexts: Contexts,
                 joint: str="",
                 can_id: int=0,
                 can_message: list[int]=[0x00] ):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param joint: Name of the joint
        :param can_id: CAN ID
        :param can_message: CAN message to send should be a valid length and of the form 0x0000 (multiple of two hex digits)
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]

        # Default joint name to the hardware interface name
        if len(joint) == 0:
            joint = self.name

        self.declare_parameter("joint", joint, "Name of the joint")
        self.declare_parameter("can_id", can_id, "CAN ID")
        self.declare_parameter("can_message", can_message, "CAN message to send should be a valid length and of the form 0x0000 (multiple of two hex digits)")

        # Setup event callback.
        if EventCollection in contexts:
            events = contexts[EventCollection]
            events[f"{self.get_parameter("joint").value}/trigger"].add_callback(self.on_trigger)
        else:
            self.logger.error("Could not find EventCollection in the python control contexts, cannot be triggered.")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        """ Used to set up your HardwareInterface. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Update params
        self.can_id: int = self.get_parameter("can_id").value
        self.can_message: list[int] = self.get_parameter("can_message").value
        self.joint: str = self.get_parameter("joint").value
        return True

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

    def on_trigger(self):
        """ Called whenever the trigger event is invoked. """
        try:
            frame = jcan.Frame(id=self.can_id, data=self.can_message)
            self.bus.send(frame)
        except Exception as e:
            self.logger.error(f"Failed to send jcan message: {e}")

