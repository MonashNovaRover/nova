
This part of the documentation should teach people ROS2 Control.

It realize the methodology/mindset for writing control loops in the previous chapter. 

# ros2_control

In [the previous chapter](./01_programming_control_loops.md), I talked about what control loop software should theoretically look like. [ROS2 Control](https://control.ros.org/jazzy/index.html) is the framework we use to actually do this, which provides all the abstractions I discussed earlier.

The new arm control code is built upon the [ros2_control](https://control.ros.org/jazzy/index.html) framework.

> The ros2_control is a framework for (real-time) control of robots using ([ROS 2](https://docs.ros.org/en/rolling/)).
> ros2_control’s goal is to simplify integrating new hardware and overcome some drawbacks.
>
> If you are not familiar with the control theory, please get some idea about it (e.g., at
> [Wikipedia](https://en.wikipedia.org/wiki/Control_theory)) to get familiar with the terms used in this manual.

There isn't very straightforward introductory documentation on ros2_control for getting started with the basic concepts,
so this document is being written to compensate.

---

We are writing a high-level discrete time control loop. This is a big loop that runs some fixed number `n` times per second which:

1. Reads the current state of the physical system
2. Calculates target states for the physical system
    - based on
        - states from the physical system
        - external inputs, such as from a gamepad or joysticks
3. Writes target states to the physical system
    - The physical system will try to achieve that desired state

State in this context often refers to the position or velocity of some motor.

ros2_control is a framework for building up these control loops using different components.

## Interfaces

Not to be confused with the other interfaces from ROS2,

Interfaces in ros2_control are 











