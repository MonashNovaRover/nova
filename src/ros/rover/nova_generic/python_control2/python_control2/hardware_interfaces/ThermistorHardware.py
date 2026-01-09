import jcan
from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts

class ThermistorHardware(HardwareInterface):
    # temperature_state: Interface
    # can_id: int
    # sensor_name: str

    def __init__(self, contexts: Contexts,
                 sensor_name: str= "",
                 can_id: int=0,
                 beta_0: int=80.797,
                 beta_1: int=-0.0169):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param can_id: CAN ID of message containing temperature from thermistor
        :param beta_0: calibration constant in temperature = beta_0 + beta_1 * received_data
        :param beta_1: calibration constant in temperature = beta_0 + beta_1 * received_data
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]
        self.last_temperature = 0

        # Default sensor name to the hardware interface name
        if len(sensor_name) == 0:
            sensor_name = self.name

        self.declare_parameter("sensor_name", sensor_name)
        self.declare_parameter("can_id", can_id)
        self.declare_parameter("beta_0", beta_0)
        self.declare_parameter("beta_1", beta_1)

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
        self.sensor_name: str = self.get_parameter("sensor_name").value
        self.can_id: int = self.get_parameter("can_id").value
        self.beta_0: float = self.get_parameter("beta_0").value
        self.beta_1: float = self.get_parameter("beta_1").value

        # Get state interface
        self.temperature_state: Interface[float] = state_interfaces[self.sensor_name + "/temperature"]

        # Validate state interface configuration
        if not self.temperature_state:
            self.logger.warn(f"ThermistorHardware \"{self.name}\" has no populated state interface. "
                             f"(\"{self.sensor_name}/temperature\")")

        self.bus.add_callback(self.can_id, self.frame_callback)

        return True

    def frame_callback(self, frame: jcan.Frame):
        if len(frame.data) != 2:
            self.logger.warn(f"ThermistorHardware \"{self.name}\" CAN data expected 2 bytes "
                             f"but got {len(frame.data)} instead")

        raw_data = int.from_bytes([frame.data[-2], frame.data[-1]])

        self.last_temperature = self.beta_0 + self.beta_1 * raw_data

    def on_read(self, now: float, period: float):
        """ Called to read values from hardware, and put them into stored state interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        self.temperature_state.value = self.last_temperature

    def on_write(self, now: float, period: float):
        """ Called to write to hardware using values stored in command interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass
