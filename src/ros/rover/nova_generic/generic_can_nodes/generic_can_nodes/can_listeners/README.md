# Generic CAN Listeners

A set of generic nodes that listen to the CAN bus and publish what they receive to a topic.

These were initially intended to be used as a generic sensor class for science but can be applied to any situation.

## Current Variations

- **CAN Number Listener**: Converts anything it receives into a number, can apply a basic linear scale and offset.
  - Publishes `generic_interface/Float64.msg`.
  - Designed for integer sensors (eg temperature), and transmitting raw messages from the bus.
- **CAN Boolean Listener**: Converts anything it receives into a boolean value.
  - Publishes `generic_interfaces/Bool.msg`
  - Designed for hall effect sensors that are either triggered or not. 

## Example Usage

Here is an example of using a CAN Number Listener as a temperature sensor where the CAN message is in Kelvin, and we want to publish data in Celsius.

First add the node to a launch file:

```python
Node(
    package='generic_can_nodes',
    executable='can_listeners/CANNumberListener.py',
    name='TemperatureSensor',
    parameters=[param_file],
    output='screen',
    emulate_tty=True,
)
```

Then add config:

```yaml
TemperatureSensor:
  ros__parameters:
    frame_id: 0x0A0
    topic: '/sensor/temp'
    offset: −273.15
```

To see all config options, please look at the corresponding [parameter definition files](./generic_can_nodes/can_listeners/config).

## Development

If you have an idea for a new generic can listener feel free to add, however if you have a specific circumstance with a specific mapping you can extend or develop one of the current listeners.

### Example 1: Float -> custom logic -> Float

For example you want to apply a custom calibration function to convert raw CAN data to a useful value.

<details markdown="1">
<summary>
Example Code
</summary> 

```python
#!/usr/bin/env python3

import rclpy
from generic_can_nodes.can_listeners.CANNumberListener import CANNumberListener

class CustomCANListener(CANNumberListener):
    """Class to represent a CAN Listener that processes numbers"""

    def __init__(self):
        super().__init__(name="CustomCanListener")

    def mapper(self, num: float) -> float:
        """ Does some custom logic """
        
        # do some custom logic
        
        return result

def main():
    rclpy.init()
    custom_can_listener = CustomCANListener()
    rclpy.spin(custom_can_listener)
    rclpy.shutdown()

if __name__ == "__main__":
    main()

```

</details>

### Example 2: jcan.Frame -> custom logic -> Custom Msg

For implementing a custom child class of `CANListener`. 
See `CANBoolListener` and `CANNumberListener` for examples on how to do this.

 custom logic within the CANListener - see `DynamicCANListener` for an example on how to do this.
 