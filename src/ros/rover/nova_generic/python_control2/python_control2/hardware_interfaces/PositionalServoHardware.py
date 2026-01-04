import jcan

from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts

class PositionalServoHardwareHandle:
    def __init__(self, min_value: float, max_value: float):

        self.min_value = min_value
        self.min_value_can: int=0x00
        self.max_value = max_value
        self.max_value_can: int=0xFF

    def send_value(self, bus: jcan.Bus, frame_id: int, function_id: int, value: float):
        data = self.convert_to_can(value)
        bus.send(jcan.Frame(frame_id, [function_id, data]))

    def convert_to_can(self, value: float) -> int:
        data: int = 0

        if value >= self.max_value:
            data = self.max_value_can
        elif (value <= self.min_value) or (self.max_value <= self.min_value):
            data = self.min_value_can
        else:
            data = int(self.max_value_can * (value - self.min_value) / (self.max_value - self.min_value))

        return data

class PositionalServoHardware(HardwareInterface):
    pos_cmd: Interface
    position_handle: PositionalServoHardwareHandle
    frame_id: int
    function_id: int
    # The name of the joint
    joint: str

    def __init__(self, contexts: Contexts,
                 joint: str="",
                 frame_id: int=0x0A0,
                 function_id: int=0x01,
                 min_angle: float=0.0,
                 max_angle: float=180.0):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]

        # Default joint name to the hardware interface name
        if len(joint) == 0:
            joint = self.name

        self.declare_parameter("joint", joint)
        self.declare_parameter("frame_id", frame_id)
        self.declare_parameter("function_id", function_id)
        self.declare_parameter("min_angle", min_angle)
        self.declare_parameter("max_angle", max_angle)

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
        self.frame_id: int = int(self.get_parameter("frame_id").value)
        self.function_id: int = int(self.get_parameter("function_id").value)
        self.joint: str = self.get_parameter("joint").value

        min_angle = float(self.get_parameter("min_angle").value)
        max_angle = float(self.get_parameter("max_angle").value)
        self.position_handle = PositionalServoHardwareHandle(min_angle, max_angle)

        # Get command interfaces
        self.pos_cmd = command_interfaces[self.joint + "/position"]

        # Validate command interface configuration
        if not self.pos_cmd:
            self.logger.warn(f'PositionalServoHardware "{self.name}" has no populated command interface '
                             f'("{self.joint}/position")')

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
        if self.pos_cmd:
            self.position_handle.send_value(self.bus, self.frame_id, self.function_id, float(self.pos_cmd.value))
