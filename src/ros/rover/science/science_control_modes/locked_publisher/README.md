# locked_publisher/LockedPublisher

Control Mode that publishes the locked status of Teleop Modular.

Publishes [teleop_msgs/LockedStatus](../../teleop_msgs/msg/LockedStatus.msg) messages.

Created using [control_mode_template](https://github.com/BaileyChessum/control_mode_template).

### Parameters

*TODO: Describe your parameters here*

- `topic : string` The topic name to send messages to (Required)
- `qos : int` The ROS2 topic Quality of Service value to use in the publisher (Optional, defaults to 10) 

```yaml
# Example parameter file
teleop_node:
  ros__parameters:
    control_modes:
      names: [ "locked_publisher" ]
      locked_publisher:
        type: "locked_publisher/LockedPublisher"

locked_publisher:
  ros__parameters:
    # Topic to send messages to (Required)
    topic: "/turtle1/cmd_vel"
    
    # The ROS2 topic Quality of Service value to use in the publisher (Optional, defaults to 1, reliable)
    qos:
      depth: 1
      reliable: false   #< do this to use best_effort
```
