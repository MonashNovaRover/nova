"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware interface for a carousel ring
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
This hardware interface manages a carousel ring servo
with position control, zeroing capability, multi-sensor
feedback, and zero offset adjustment.

Composes PositionalServoHardware for servo control and
MultiSensorHardware for CAN-based sensor feedback.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - carousel/position    [target position in degrees, 0 to angular_limit]
STATE INTERFACES:
  - carousel/position    [actual position in degrees with zero offset applied]
  - <name>/sensor_position   [position feedback from hardware sensor]
  - <name>/sensor_load       [load feedback from hardware]
  - <name>/sensor_current    [current feedback from hardware]
  - <name>/zeroing           [boolean, True while zeroing is active]
SERVICES:
  - science/<name>/trigger_zero    [Trigger service to initiate hardware zeroing]
  - science/<name>/increment_zero  [IncrementZero service to adjust or reset software zero offset]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Felicity Matthews
CREATION:       19/04/26
EDITED:         19/04/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import jcan
from python_control2 import Interface, InterfaceCollection, HardwareInterface, Contexts
from python_control2.hardware_interfaces import PositionalServoHardware, MultiSensorHardware
from std_srvs.srv import Trigger
from science_interfaces.srv import IncrementZero


ANGULAR_LIMIT = 360


class CarouselHardware(HardwareInterface):
    target_pos_cmd: Interface
    forward_pos_cmd: Interface
    actual_pos_state: Interface
    forward_pos_state: Interface
    zeroing_in_progress_state: Interface

    def __init__(self, contexts: Contexts,
                 zero_cmd_can_id: int=0x000,
                 zero_cmd_can_msg: list[int]=[0x00],
                 zero_rec_can_id: int=0x000,
                 zero_rec_can_msg: list[int]=[0x00],
                 zero_service: str="science/carousel/trigger_zero",
                 zero_increment_service: str="science/carousel/increment_zero"):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        # Get CAN bus
        self.bus = contexts[jcan.Bus]

        # Initialize zero offset
        self.zero_offset = 0.0
        self.zeroing_in_progress = False

        # Declare Parameters
        self.declare_parameter("zero_cmd_can_id", zero_cmd_can_id, "CAN ID of the zero command")
        self.declare_parameter("zero_cmd_can_msg", zero_cmd_can_msg, "CAN message to send should be a valid length and of the form 0x0000 (multiple of two hex digits)")
        self.declare_parameter("zero_rec_can_id", zero_rec_can_id, "CAN ID of the zero command")
        self.declare_parameter("zero_rec_can_msg", zero_rec_can_msg, "CAN message to send should be a valid length and of the form 0x0000 (multiple of two hex digits)")
        trigger_service_name = self.declare_parameter("zero_service", zero_service, "Service that triggers the hardware to zero").value
        increment_service_name = self.declare_parameter("zero_increment_service", zero_increment_service, "Service to increment the software zero offset").value

        # Create services
        self.node.create_service(Trigger, trigger_service_name, self._trigger_zero_callback)
        self.node.create_service(IncrementZero, increment_service_name, self._increment_zero_callback)

        # Initialise composed hardware interfaces using DeferredConstructor pattern
        # Calling the class returns a DeferredConstructor, then we set name and call construct()
        servo_constructor = PositionalServoHardware(angular_limit=360, function_id=0x01, packed_data_length=2, max_angle_can=0xFFFF)
        servo_constructor.name = "servo"
        self.servo = servo_constructor.construct(contexts, self.node)

        # Position feedback sensor
        position_sensor_constructor = MultiSensorHardware(
            function_id=0x01,
            interpret_data_list=[lambda data: int.from_bytes(data[0:2], byteorder='big', signed=True)],
            hardware_names=[f"{self.name}"],
            hardware_units=["sensor_position"],
            initial_values=[0]
        )
        position_sensor_constructor.name = f"{self.name}_position_sensor"
        self.position_sensor = position_sensor_constructor.construct(contexts, self.node)

        # Load and current feedback sensor
        load_current_sensor_constructor = MultiSensorHardware(
            function_id=0x02,
            interpret_data_list=[
                lambda data: int.from_bytes(data[0:2], byteorder='big', signed=True),  # Load
                lambda data: int.from_bytes(data[2:4], byteorder='big', signed=True)   # Current
            ],
            hardware_names=[f"{self.name}", f"{self.name}"],
            hardware_units=["sensor_load", "sensor_current"],
            initial_values=[0, 0]
        )
        load_current_sensor_constructor.name = f"{self.name}_load_current_sensor"
        self.load_current_sensor = load_current_sensor_constructor.construct(contexts, self.node)

    def _zero_complete_can_callback(self, frame: jcan.Frame):
        """ CAN callback for zeroing completion message """
        zero_rec_can_msg = self.get_parameter("zero_rec_can_msg").value

        # Check if the received message matches the expected zeroing complete message
        if list(frame.data) == zero_rec_can_msg:
            # Capture current position as new zero offset
            if self.actual_pos_state:
                self.zero_offset = self.actual_pos_state.value
                self.logger.info(f"{self.name} zero offset set to {self.zero_offset}")

            self.zeroing_in_progress = False
            self.logger.info(f"{self.name} zeroing complete")

    def _trigger_zero_callback(self, request, response):
        """ Service callback to trigger zeroing by sending CAN message """
        try:
            # Get parameters
            zero_cmd_can_id = self.get_parameter("zero_cmd_can_id").value
            zero_cmd_can_msg = self.get_parameter("zero_cmd_can_msg").value

            # Set zeroing in progress flag
            self.zeroing_in_progress = True

            # Send zero command via CAN
            frame = jcan.Frame(zero_cmd_can_id, zero_cmd_can_msg)
            self.bus.send(frame)

            response.success = True
            response.message = "Zero command sent successfully"
            self.logger.info(f"{self.name} zeroing started")
        except Exception as e:
            response.success = False
            response.message = f"Failed to send zero command: {str(e)}"
            self.logger.error(f"Error in trigger_zero: {e}")

        return response

    def _increment_zero_callback(self, request, response):
        """ Service callback to increment or reset zero offset """
        try:
            if request.reset_zero:
                self.zero_offset = 0.0
                self.logger.info("Zero offset reset to 0.0")
            else:
                self.zero_offset += request.increment_zero
                self.logger.info(f"Zero offset incremented by {request.increment_zero}, new offset: {self.zero_offset}")

            response.success = True
        except Exception as e:
            response.success = False
            self.logger.error(f"Error in increment_zero: {e}")

        return response

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        """ Used to set up your HardwareInterface. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Get interfaces FIRST to mark them as populated before configuring composed hardware
        self.target_pos_cmd = command_interfaces["carousel/position"]
        self.forward_pos_cmd = command_interfaces["servo/position"]
        self.forward_pos_cmd.populated = True
        self.actual_pos_state = state_interfaces[f"{self.name}/sensor_position"]
        self.forward_pos_state = state_interfaces["carousel/position"]
        self.zeroing_in_progress_state = state_interfaces[f"{self.name}/zeroing"]

        # Now configure composed hardware interfaces (they'll find their interfaces already populated)
        result = self.servo.on_configure(command_interfaces, state_interfaces)
        if result is False:
            return False

        result = self.position_sensor.on_configure(command_interfaces, state_interfaces)
        if result is False:
            return False

        result = self.load_current_sensor.on_configure(command_interfaces, state_interfaces)
        if result is False:
            return False

        # Register CAN callback for zeroing completion
        zero_rec_can_id = self.get_parameter("zero_rec_can_id").value
        self.bus.add_callback(zero_rec_can_id, self._zero_complete_can_callback)

        return True

    def on_read(self, now: float, period: float):
        """ Called to read values from hardware, and put them into stored state interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Call composed hardware interface read
        self.servo.on_read(now, period)
        self.position_sensor.on_read(now, period)
        self.load_current_sensor.on_read(now, period)

        # Update carousel position state from servo with inverse zero offset applied
        if self.actual_pos_state:
            raw_value = self.actual_pos_state.value - self.zero_offset
            self.forward_pos_state.value = raw_value % ANGULAR_LIMIT

        # Update zeroing in progress state
        if self.zeroing_in_progress_state:
            self.zeroing_in_progress_state.value = self.zeroing_in_progress

    def on_write(self, now: float, period: float):
        """ Called to write to hardware using values stored in command interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Forward target position to servo with zero offset applied
        if self.target_pos_cmd:
            raw_value = self.target_pos_cmd.value + self.zero_offset
            self.forward_pos_cmd.value = raw_value % ANGULAR_LIMIT

        # Call composed hardware interface write
        self.servo.on_write(now, period)