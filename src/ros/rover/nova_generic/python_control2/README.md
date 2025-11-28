# Python Control 2

## Concepts

### Controllers

Controllers encapsulate high level control of hardware. They can convert input (e.g. from joysticks or the GUI) to 
hardware commands (e.g. move the motor at 5 m/s). They can also take data from the hardware (e.g. temperature readings) 
and use that information to, for example, display on the GUI or use to make decisions about it's commands.

The Controller abstract class can be found [here](./python_control2/controllers/Controller.py).

### Hardware Interfaces

Hardware interfaces represent the low level control that interacts with the hardware. This can involve taking commands 
from the controllers (e.g. move the motor at 5m/s) and interacting with the hardware to make that happen. For us that 
often means taking a velocity or effort and sending the corresponding CAN commands that achieve that functionality. 
Hardware interfaces can also read data from hardware (e.g. temperature sensors) and pass that on to controllers.

The Hardware Interface abstract class can be found [here](./python_control2/hardware_interfaces/HardwareInterface.py).

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

### Controller Manager

The controller manager is what runs a Python Control 2 system, it has references to all of the above and manages the update
loop.

## How to Use

For an example implementation please see [`control_test.py`](../../science/science/science/control_test.py).

[Cookiecutter templates](https://github.com/MonashNovaRover/nova-templates) also exist to help you start out. Currently, there are templates for:
- [Controllers](https://github.com/MonashNovaRover/nova-templates/tree/master/python-control2-controller)

### Builder 

Python Control 2 uses a builder pattern to build a Python control **system**.

Please have a look at the [inline documentation](./python_control2/controller_manager/ControllerManagerBuilder.py) for detailed 
information about each available builder option.

```python
# Node with the system name
node = Node("control_test")
# If using teleop modular create a python utils inputs
inputs = Inputs(node).with_topics("/science/input")

# PythonControl takes in the node, and a list of default ros2 parameter values
PythonControl(node, update_rate=5, can_bus="can1") \
    # Add controllers with a name and class, and any parameters to be passed into the __init__ method.
    .with_controller("test_controller", TestController, joint="j1") \
    # Add hardware interfaces with a name and class, and any parameters to be passed into the __init__ method.
    .with_hardware("test_hw", TestHardware) \
    .with_hardware("j1_cmd", CMDHardware, "j1", can_id=0x1) \
    .with_hardware("j2_cmd", CMDHardware, "j2", can_id=0x1F) \
    .with_hardware("j3_cmd", CMDHardware, "j3", can_id=0x043) \
    # Add teleop support
    .with_teleop(inputs) \
    # Add JCAN, will be accessible by all controller/hardware in a system
    .with_jcan()
    # Adds a context to the control managers contexts which can be accessed in all __init__ methods.
    .with_context() \
    .spin()
```

### Contexts

Contexts is what is used to share instances around a Python Control 2 system. For example, you only what one jcan Bus,
but you may have multiple Hardware Interfaces that all need access to it.

The Controller Manager has a Contexts which gets added to during the building process, and is then passed to each Controller/Hardware
Interface upon initialisation, where they can obtain a reference to the resource. Resources are accessed and stored via their Class.

The `.with_context(item)` builder option should be used if multiple Controller/Hardware need access to a shared resource.

### Teleop Modular Integration

Python Control 2 support [`teleop_modular`](https://github.com/BaileyChessum/teleop_modular). Please see the documentation
there on how `teleop_modular` works.

Python Control 2 works with `teleop_modular` through `Inputs` which you can read the documentation for 
[here](https://github.com/BaileyChessum/teleop_modular/blob/main/teleop_python_utils/teleop_python_utils/modules/Inputs.py).
Again please see [`control_test.py`](../../science/science/science/control_test.py) for example of how this can be used.


