# generic_can_hardware

This is a custom `ros2_control` hardware interface used to control some generic CAN device. 
It communicates directly with any active `ros2_control` controllers via the Controller Manager.

Consult `ros2_control` [documentation](https://control.ros.org/rolling/doc/ros2_control/hardware_interface/doc/writing_new_hardware_component.html) on how to write a hardware interface.

### Planned vision

This behaviour is currently not yet implemented, but describes the current vision.

The aim is to make a hardware interface which can be widely applied to many simple I/O devices running on CAN, such as the Kiln.

The behaviour of the hardware interface is determined entirely by the `state_interface` and `command_interface` definitions and their parameters in the URDF.

```xacro
<ros2_control name="ARCScienceKiln" type="sensor">
  <hardware>
    <plugin>generic_can_hardware/GenericCANHardware</plugin>
    <param name="candevice">can0</param>        <!-- The CAN bus to use -->
  </hardware>
  <gpio name="kiln">
    <state_interface name="temperature">
      <param name="canid">0x0A0</param>         <!-- The CAN ID to read messages from -->
      <param name="data_type">float64</param>   <!-- The type of the data CAN messages represent -->
      <!-- params for a float64 conversion: -->
      <param name="min">-273.15</param>         <!-- What a CAN message with all 0s should correspond to -->
      <param name="max">2348.27</param>         <!-- What a CAN message with all 1s should correspond to -->
      <param name="can_msg_bytes">2</param>     <!-- The number of bytes in the CAN message. Determines the max CAN value. -->
    </state_interface>
    <command_interface name="heating">
      <param name="canid">0x0A0, 0x0B0</param>  <!-- The CAN IDs to send messages to (comma separated) -->
      <param name="data_type">bool</param>      <!-- The type of the data CAN messages represent -->
    </command_interface>
  </gpio>
</ros2_control>
```

Parameters such as `<param name="data_type"/>` determine the translation between meaningful data in `ros2_control` and the raw data inside of a CAN message, in addition to some additional parameters dependending on the value of `<param name="data_type"/>`.

Each data type needs to be manually supported, with names based on those from [ROS2 interface types](https://docs.ros.org/en/jazzy/Concepts/About-ROS-Interfaces.html#field-types). Currently supported types include:
- `float64` (`double`)

Note: With CAN classic, CAN messages can contain up to 8 bytes per message, but this can increase when the team introduces CAN FD, which can include up to 64 bytes.
