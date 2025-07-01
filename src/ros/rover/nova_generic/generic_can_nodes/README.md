# Generic Can Nodes

Package for nodes that serve a generic purpose interacting with the CAN bus.

## Generic CAN Listeners

A set of generic nodes that listen to the CAN bus and publish what they receive.

These were initally intended to be used as a generic sensor class for science but can be applied to any situation.

### Current Variations

- **CAN Number Listener**: Converts anything it receives into a number, can apply a basic linear scale and offset.
  - Designed for integer sensors (eg temperature), and transmitting raw messages from the bus.
- **CAN Boolean Listener**: Converts anything it receives into a boolean value.
  - Designed for hall effect sensors that are either triggered or not. 

### Example Usage

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
