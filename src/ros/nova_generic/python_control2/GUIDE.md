# How to Write a Python Control2 System

This guide walks you through creating a Python Control2 system from scratch, with practical examples and patterns.

## Table of Contents

1. [Quick Start](#quick-start)
2. [Architecture Overview](#architecture-overview)
3. [Writing a Custom Controller](#writing-a-custom-controller)
4. [Writing a Custom Hardware Interface](#writing-a-custom-hardware-interface)
5. [Using Built-in Components](#using-built-in-components)
6. [Adding ROS Services and Topics](#adding-ros-services-and-topics)
7. [Adding ROS Action Servers](#adding-ros-action-servers)
8. [Teleop Integration](#teleop-integration)
9. [Activation System](#activation-system)
10. [Common Patterns](#common-patterns)
11. [Troubleshooting](#troubleshooting)

---

## Quick Start

Here's a minimal Python Control2 system that controls a motor:

```python
#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from python_control2 import PythonControl
from python_control2.controllers import EffortCommandController
from python_control2.hardware_interfaces import QCMDHardware

def main():
    rclpy.init()
    node = Node("my_motor")

    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller(
            "controller",
            EffortCommandController,
            hardware_name="motor",
            service_name="/my_motor/command",
            topic_name="/my_motor/status"
        ) \
        .with_hardware("motor", QCMDHardware, can_id=0x042) \
        .with_jcan() \
        .spin()

if __name__ == "__main__":
    main()
```

This creates a system where:
- A motor at CAN ID `0x042` can be controlled via the `/my_motor/command` service
- Status is published to `/my_motor/status`
- The system updates at 10Hz

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    PythonControl System                     │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐         Command         ┌───────────────┐ │
│  │              │        Interfaces       │               │ │
│  │  Controller  │ ──────────────────────► │   Hardware    │ │
│  │              │                         │   Interface   │ │
│  │  (on_update) │ ◄────────────────────── │               │ │
│  │              │         State           │  (on_read)    │ │
│  └──────────────┘        Interfaces       │  (on_write)   │ │
│         │                                 └───────┬───────┘ │
│         │                                         │         │
│         ▼                                         ▼         │
│  ┌──────────────┐                         ┌───────────────┐ │
│  │ ROS Topics/  │                         │   CAN Bus     │ │
│  │ Services/    │                         │   (jcan)      │ │
│  │ Actions      │                         └───────────────┘ │
│  └──────────────┘                                           │
└─────────────────────────────────────────────────────────────┘
```

### Update Cycle (runs at `update_rate` Hz)

1. **on_read** - Hardware interfaces read from CAN, update state interfaces
2. **on_update** - Controllers read state, compute commands, write to command interfaces
3. **on_write** - Hardware interfaces read commands, send to CAN

### Key Concepts

| Concept | Purpose | Example |
|---------|---------|---------|
| **Controller** | High-level logic | Convert joystick input to motor effort |
| **Hardware Interface** | Low-level hardware communication | Send effort as CAN frame |
| **Command Interface** | Commands from controller to hardware | `motor/effort` = 0.75 |
| **State Interface** | Sensor data from hardware to controller | `sensor/temperature` = 25.0 |
| **Contexts** | Dependency injection | Access CAN bus, teleop inputs |

---

## Writing a Custom Controller

### Basic Structure

```python
from typing import Optional
from python_control2 import Controller, Contexts, InterfaceCollection, Interface

class MyController(Controller):

    def __init__(self, contexts: Contexts,
                 my_param: str = "default_value"):
        """
        Constructor - called during system setup.

        :param contexts: Dependency injection container
        :param my_param: Your custom parameter
        """
        super().__init__(contexts)

        # Declare ROS parameters (can be overridden via launch file)
        self.my_param = self.declare_parameter("my_param", my_param).value

        # Initialize state
        self.some_state = 0.0

        self.logger.info(f"MyController initialized with param: {self.my_param}")

    def on_configure(self, command_interfaces: InterfaceCollection,
                     state_interfaces: InterfaceCollection) -> Optional[bool]:
        """
        Called once before the update loop starts.
        Get your interfaces and create ROS publishers/services here.

        :returns: True if configured successfully, False otherwise
        """
        # Get command interfaces (what you write to)
        self.motor_cmd = command_interfaces["motor/effort"]

        # Get state interfaces (what you read from)
        self.temp_state = state_interfaces["sensor/temperature"]

        return True

    def on_update(self, now: float, period: float):
        """
        Called every update cycle.
        Read state, compute, write commands.

        :param now: Current time in seconds
        :param period: Time since last update in seconds
        """
        # Read sensor data
        temperature = self.temp_state.value

        # Compute command
        if temperature < 50.0:
            effort = 1.0
        else:
            effort = 0.0

        # Write command
        self.motor_cmd.value = effort
```

### Available in Controllers

| Attribute | Type | Description |
|-----------|------|-------------|
| `self.name` | `str` | Controller name (from builder) |
| `self.node` | `Node` | ROS2 node reference |
| `self.logger` | `Logger` | Hierarchical logger |
| `self.declare_parameter()` | method | Declare ROS parameter |
| `self.get_parameter()` | method | Get ROS parameter |

### Parameter Naming

Parameters are automatically namespaced: `controllers.<name>.<param_name>`

```python
# In controller
self.declare_parameter("speed", 1.0)

# In launch file or CLI
--ros-args -p controllers.my_controller.speed:=2.0
```

---

## Writing a Custom Hardware Interface

### Basic Structure

```python
import jcan
from typing import Optional
from python_control2.hardware_interfaces import HardwareInterface
from python_control2 import Contexts, InterfaceCollection, Interface

class MyHardware(HardwareInterface):

    def __init__(self, contexts: Contexts,
                 can_id: int = 0x000,
                 some_option: bool = False):
        """
        Constructor - called during system setup.
        """
        super().__init__(contexts)

        # Get CAN bus from contexts
        self.bus = contexts[jcan.Bus]

        # Declare parameters
        self.can_id = self.declare_parameter("can_id", can_id).value
        self.some_option = self.declare_parameter("some_option", some_option).value

    def on_configure(self, command_interfaces: InterfaceCollection,
                     state_interfaces: InterfaceCollection) -> Optional[bool]:
        """
        Get interfaces. Hardware interfaces typically:
        - Read from command interfaces
        - Write to state interfaces
        """
        # Command interface: controller writes, we read
        self.effort_cmd = command_interfaces[f"{self.name}/effort"]

        # State interface: we write, controller reads
        self.position_state = state_interfaces[f"{self.name}/position"]

        # Register CAN callback for receiving data
        self.bus.add_callback(self.can_id, self.on_can_receive)

        return True

    def on_can_receive(self, frame: jcan.Frame):
        """Called when CAN frame received matching our ID."""
        # Parse position from CAN data
        position = (frame.data[0] << 8) | frame.data[1]
        self.position_state.value = position / 1000.0  # Convert to meters

    def on_read(self, now: float, period: float):
        """
        Called before controllers update.
        Read from hardware, update state interfaces.
        """
        # jcan.Bus.spin() is called automatically before on_read
        # CAN callbacks have already updated state interfaces
        pass

    def on_write(self, now: float, period: float):
        """
        Called after controllers update.
        Read command interfaces, send to hardware.
        """
        # Get effort from controller
        effort = self.effort_cmd.value

        # Convert to CAN data (example: signed 16-bit)
        data_int = int(effort * 32767)
        data = [(data_int >> 8) & 0xFF, data_int & 0xFF]

        # Send CAN frame
        frame = jcan.Frame(id=self.can_id, data=data)
        self.bus.send(frame)
```

### Parameter Naming

Hardware parameters are namespaced: `hardware.<name>.<param_name>`

---

## Using Built-in Components

### Built-in Controllers

| Controller | Purpose | Parameters |
|------------|---------|------------|
| `EffortCommandController` | Service-based effort control | `hardware_name`, `service_name`, `topic_name` |
| `ActuateController` | Joystick-to-effort | `hardware_name`, `actuation_axis` |
| `PresetTwitchController` | Preset positions with fine adjustment | `positions`, `hardware_name`, services |

### Built-in Hardware Interfaces

| Hardware Interface | Purpose | Key Parameters |
|-------------------|---------|----------------|
| `QCMDHardware` | QCMD motor drivers (effort) | `can_id`, `max_effort`, `reversed` |
| `CMDHardware` | CMD motor drivers (velocity/effort) | `can_id`, `drive_type` |
| `PositionalServoHardware` | Positional servos | `can_id`, `angular_limit`, `gear_ratio` |
| `ContinousServoHardware` | Continuous rotation servos | `can_id` |
| `GenericSensorHardware` | Generic sensor reading | `can_id`, `interpret_data`, `unit` |
| `MultiSensorHardware` | Multiple values from one CAN ID | `can_id`, `channels` |
| `TriggerHardware` | Send CAN on event trigger | `can_id`, `can_message` |

### Example: Using QCMDHardware

```python
PythonControl(node, update_rate=10, can_bus="can1") \
    .with_controller("ctrl", EffortCommandController,
                     hardware_name="pump",
                     service_name="/pump/command",
                     topic_name="/pump/status") \
    .with_hardware("pump", QCMDHardware,
                   can_id=0x0D1,
                   max_effort=0.75,
                   reversed=False) \
    .with_jcan() \
    .spin()
```

### Example: Using GenericSensorHardware

```python
# Custom data interpreter
def parse_temperature(data: list[int]) -> float:
    raw = (data[0] << 8) | data[1]
    return raw / 100.0 - 40.0  # Convert to Celsius

PythonControl(node, update_rate=5, can_bus="can1") \
    .with_controller("ctrl", MyController) \
    .with_hardware("temp_sensor", GenericSensorHardware,
                   can_id=0x4E1,
                   interpret_data=parse_temperature,
                   initial_value=0.0,
                   unit="temperature") \
    .with_jcan() \
    .spin()
```

---

## Adding ROS Services and Topics

Create services and publishers in `on_configure()`:

```python
from my_interfaces.srv import MyService
from my_interfaces.msg import MyMessage

class MyController(Controller):

    def on_configure(self, command_interfaces, state_interfaces):
        # Create a service
        self.service = self.node.create_service(
            MyService,
            "/my_service",
            self.service_callback
        )

        # Create a publisher
        self.publisher = self.node.create_publisher(
            MyMessage,
            "/my_topic",
            10  # QoS depth
        )

        # Create a timer for periodic publishing
        self.pub_timer = self.node.create_timer(
            1.0 / 5,  # 5 Hz
            self.publish_status
        )

        return True

    def service_callback(self, request, response):
        """Handle service requests."""
        self.some_state = request.value
        response.success = True
        return response

    def publish_status(self):
        """Periodic status publication."""
        msg = MyMessage()
        msg.value = self.some_state
        self.publisher.publish(msg)
```

---

## Adding ROS Action Servers

Action servers are useful for long-running operations with feedback.

```python
from rclpy.action import ActionServer, GoalResponse, CancelResponse
from my_interfaces.action import MyAction

class MyController(Controller):

    def __init__(self, contexts):
        super().__init__(contexts)
        self.is_running = False
        self.current_goal = None

    def on_configure(self, command_interfaces, state_interfaces):
        self.motor_cmd = command_interfaces["motor/effort"]

        # Create action server
        self.action_server = ActionServer(
            self.node,
            MyAction,
            "/my_action",
            execute_callback=self.execute_callback,
            goal_callback=self.goal_callback,
            cancel_callback=self.cancel_callback
        )

        return True

    def goal_callback(self, goal_request):
        """Accept or reject goals."""
        if self.is_running:
            return GoalResponse.REJECT
        return GoalResponse.ACCEPT

    def cancel_callback(self, goal_handle):
        """Accept cancellation requests."""
        return CancelResponse.ACCEPT

    def execute_callback(self, goal_handle):
        """
        Execute the action.
        Note: This runs in a separate thread.
        """
        import time

        self.is_running = True
        self.current_goal = goal_handle

        target_time = goal_handle.request.duration
        start_time = time.time()

        while time.time() - start_time < target_time:
            if goal_handle.is_cancel_requested:
                self.motor_cmd.value = 0.0
                self.is_running = False
                goal_handle.canceled()
                return MyAction.Result(success=False)

            # Publish feedback
            feedback = MyAction.Feedback()
            feedback.progress = (time.time() - start_time) / target_time
            goal_handle.publish_feedback(feedback)

            time.sleep(0.1)

        self.is_running = False
        goal_handle.succeed()
        return MyAction.Result(success=True)

    def on_update(self, now, period):
        """Set motor effort based on action state."""
        if self.is_running:
            self.motor_cmd.value = 0.75
        else:
            self.motor_cmd.value = 0.0
```

---

## Teleop Integration

### Setup

```python
from teleop_python_utils import Inputs

node = Node("my_system")
inputs = Inputs(node).with_topics("/my_system/input")

PythonControl(node, update_rate=10, can_bus="can1") \
    .with_controller("ctrl", MyController) \
    .with_hardware("motor", QCMDHardware, can_id=0x042) \
    .with_teleop(inputs) \
    .with_jcan() \
    .spin()
```

### Using in Controller

```python
from teleop_python_utils import Inputs

class MyController(Controller):

    def __init__(self, contexts):
        super().__init__(contexts)

        # Get inputs from contexts
        inputs = contexts[Inputs]

        # Get specific controls
        self.speed_axis = inputs.get_axis("left_stick_y")
        self.enable_button = inputs.get_button("a_button")

        # Add button callbacks
        inputs.get_button("b_button").add_callback(self.on_b_pressed)

    def on_b_pressed(self):
        """Called when B button is pressed."""
        self.logger.info("B button pressed!")

    def on_update(self, now, period):
        # Read current values
        speed = self.speed_axis.value  # -1.0 to 1.0
        enabled = self.enable_button.value  # 0 or 1

        self.motor_cmd.value = speed * enabled
```

---

## Activation System

The activation system allows enabling/disabling a system via buttons.

### Setup

```python
PythonControl(node, update_rate=10, can_bus="can1") \
    .with_controller("ctrl", MyController) \
    .with_hardware("motor", QCMDHardware, can_id=0x042) \
    .with_teleop(inputs) \
    .with_activation_buttons(
        start_active=False,
        active_button_name="activate_my_system",
        inactive_button_pool_names=["activate_other_system"]
    ) \
    .with_jcan() \
    .spin()
```

### Using in Controller

```python
from python_control2.controller_manager import Activation

class MyController(Controller):

    def __init__(self, contexts):
        super().__init__(contexts)
        self.activation = contexts[Activation]

    def on_update(self, now, period):
        if not self.activation:  # Can use as boolean
            self.motor_cmd.value = 0.0
            return

        # System is active, do normal control
        self.motor_cmd.value = self.calculate_effort()
```

---

## Common Patterns

### Pattern 1: Simple Effort Control

For basic on/off or variable effort control via service:

```python
# Uses built-in controller
PythonControl(node, update_rate=10, can_bus="can1") \
    .with_controller("ctrl", EffortCommandController,
                     hardware_name="pump",
                     service_name="/pump/command",
                     topic_name="/pump/status") \
    .with_hardware("pump", QCMDHardware, can_id=0x0D1) \
    .with_jcan() \
    .spin()
```

### Pattern 2: Joystick Control

For direct joystick-to-motor control:

```python
inputs = Inputs(node).with_topics("/robot/input")

PythonControl(node, update_rate=10, can_bus="can1") \
    .with_controller("ctrl", ActuateController,
                     hardware_name="motor",
                     actuation_axis="left_stick_y") \
    .with_hardware("motor", QCMDHardware, can_id=0x042) \
    .with_teleop(inputs) \
    .with_activation_buttons(
        start_active=True,
        active_button_name="activate_drive"
    ) \
    .with_jcan() \
    .spin()
```

### Pattern 3: Sensor + Actuator (Closed Loop)

```python
class TemperatureController(Controller):
    def __init__(self, contexts, target_temp=50.0):
        super().__init__(contexts)
        self.target_temp = self.declare_parameter("target_temp", target_temp).value

    def on_configure(self, cmd_interfaces, state_interfaces):
        self.heater_cmd = cmd_interfaces["heater/effort"]
        self.temp_state = state_interfaces["sensor/temperature"]
        return True

    def on_update(self, now, period):
        current_temp = self.temp_state.value
        if current_temp < self.target_temp:
            self.heater_cmd.value = 1.0
        else:
            self.heater_cmd.value = 0.0

# Build system
PythonControl(node, update_rate=5, can_bus="can1") \
    .with_controller("ctrl", TemperatureController, target_temp=60.0) \
    .with_hardware("heater", QCMDHardware, can_id=0x0C1) \
    .with_hardware("sensor", GenericSensorHardware,
                   can_id=0x4E1,
                   interpret_data=lambda d: ((d[0] << 8) | d[1]) - 273.15,
                   initial_value=20.0,
                   unit="temperature") \
    .with_jcan() \
    .spin()
```

### Pattern 4: Multiple Motors

```python
class DualMotorController(Controller):
    def on_configure(self, cmd_interfaces, state_interfaces):
        self.left_cmd = cmd_interfaces["left_motor/effort"]
        self.right_cmd = cmd_interfaces["right_motor/effort"]
        return True

    def on_update(self, now, period):
        self.left_cmd.value = self.left_speed
        self.right_cmd.value = self.right_speed

# Build system
PythonControl(node, update_rate=10, can_bus="can1") \
    .with_controller("ctrl", DualMotorController) \
    .with_hardware("left_motor", QCMDHardware, can_id=0x031) \
    .with_hardware("right_motor", QCMDHardware, can_id=0x032) \
    .with_jcan() \
    .spin()
```

---

## Troubleshooting

### "Command interface not populated"

The interface name doesn't match between controller and hardware.

```python
# Hardware creates: "motor/effort"
.with_hardware("motor", QCMDHardware, can_id=0x042)

# Controller must use same name
self.cmd = command_interfaces["motor/effort"]  # Correct
self.cmd = command_interfaces["pump/effort"]   # Wrong!
```

### "No CAN messages being sent"

1. Check `.with_jcan()` is called
2. Verify CAN bus name matches your interface (`can0`, `can1`, etc.)
3. Check CAN ID is correct (use `candump` to verify)

### "Controller not receiving state updates"

1. Ensure hardware interface writes to state interface in `on_read()`
2. Check interface name matches
3. Verify sensor is sending CAN frames

---

## File Template

```python
#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: [Description of your system]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: [node_name]
TOPICS:
    - publisher: [topic] [MessageType]
SERVICES:
    - service: [service] [ServiceType]
ACTIONS:
    - [action] [ActionType]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - [hardware_name]/effort    [value range]
STATE INTERFACES:
  - [sensor_name]/[unit]      [value range]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    [package_name]
AUTHOR(S):  [Your Name]
CREATION:   [Date]
EDITED:     [Date]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface
from python_control2.hardware_interfaces import QCMDHardware


class MyController(Controller):

    def __init__(self, contexts: Contexts):
        super().__init__(contexts)
        self.logger.info(f"MyController initialized")

    def on_configure(self, command_interfaces: InterfaceCollection,
                     state_interfaces: InterfaceCollection) -> Optional[bool]:
        self.motor_cmd = command_interfaces["motor/effort"]
        return True

    def on_update(self, now: float, period: float):
        self.motor_cmd.value = 0.0


def main():
    rclpy.init()
    node = Node("my_system")

    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", MyController) \
        .with_hardware("motor", QCMDHardware, can_id=0x042) \
        .with_jcan() \
        .spin()


if __name__ == "__main__":
    main()
```

---

## Further Reading

- [README.md](./README.md) - Overview and concepts
- [Controller.py](./python_control2/controllers/Controller.py) - Controller base class
- [HardwareInterface.py](./python_control2/hardware_interfaces/HardwareInterface.py) - Hardware interface base class
- [ControllerManagerBuilder.py](./python_control2/controller_manager/ControllerManagerBuilder.py) - Builder options
- [control_test.py](../../science/science/science/control_test.py) - Example implementation
- [kiln.py](../../science/science/science/arc/kiln.py) - Complex controller example
