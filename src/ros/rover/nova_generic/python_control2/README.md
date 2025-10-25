# Python Control 2

## Concepts

### Controllers

Controllers encapsulate high level control of hardware. They can convert input (e.g. from joysticks or the GUI) to 
hardware commands (e.g. move the motor at 5 m/s). They can also take data from the hardware (e.g. temperature readings) 
and use that information to, for example, display on the GUI or use to make decisions about it's commands.

### Hardware Interfaces

Hardware interfaces represent the low level control that interacts with the hardware. This can involve taking commands 
from the controllers (e.g. move the motor at 5m/s) and interacting with the hardware to make that happen. For us that 
often means taking a velocity or effort and sending the corresponding CAN commands that achieve that functionality. 
Hardware interfaces can also read data from hardware (e.g. temperature sensors) and pass that on to controllers.

### Command interfaces

Command interfaces represent commands sent from controllers to the hardware interfaces to make it move or complete some action.

Each command interface should use standard units and conventions, the most common ones that should be used are:
- Position: what position a piece of hardware should be in (e.g. degrees, radians, cm).
- Velocity: the speed and direction a piece of hardware should move (e.g m/s in a position/negative direction, r/s).
- Effort: the force the hardware should exhort (e.g. 50%, 100%).

In general controllers **write** to command interfaces and hardware interfaces **read** from command interfaces.

### State interfaces

State interfaces represent sensor or feedback data from the hardware interfaces, allowing the controller to know the current state of the hardware.

Each state interfaces should use standard meaningful units, for example: Celsius, meters/centimetres, Pascals.

In general controllers **read** from state interfaces and hardware interfaces **write** to state interfaces.

## How to Use

### Builder 

Python Control 2 uses a builder pattern to build a Python control **system**.

```python
# PythonControl takes in the system name, and a list of default ros2 parameter values
PythonControl("control_test", update_rate=5, can_bus="can1") \
    .with_controller("test_controller", TestController, joint="j1") \
    .with_hardware("test_hw", TestHardware) \
    .with_hardware("j1_cmd", CMDHardware, "j1", can_id=0x1) \
    .with_hardware("j2_cmd", CMDHardware, "j2", can_id=0x1F) \
    .with_hardware("j3_cmd", CMDHardware, "j3", can_id=0x043) \
    .with_jcan() \
    .spin()
```
