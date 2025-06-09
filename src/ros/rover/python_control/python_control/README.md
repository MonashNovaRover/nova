# Python Control

## Overview

The python control package contains a number of classes that can be
used to abstract away both high level logic and low level communication
with the CAN bus.

There are currently five different components to the python control library. 
More detail can be found in each folder.

- **Controllers**: low level controllers of boards (constructing can messages).
- **Controls**: high level logic (which direction to move, current velocity etc).
- **Limits**: listens to the CAN bus and determines whether the limit has been hit.
- **Sensors**: listens to the CAN bus for data and recorded the latest sensor value.
- **Controller Nodes**: abstracts away some common functionality and allows controllers to send constant CAN messages.

## Controller Nodes

If using this library and/or interacting the CAN bus a Controller Node should be used.
A Controller Node starts the CAN bus and calls each controller's callback function every x seconds.

There also exists two types of Controller Nodes if the joysticks are being used to control.

- **JoystickControllerNode**: Listens to joystick messages and reveals a function that will be called
everytime a message is received from each joystick. Should be used when input from joysticks is required.
- **ActivatedJoystickControllerNode**: Is a Joystick Controller Node that has the added ability to be 
de/activated based on buttons pressed on the joystick, allows the joystick to be used for multiple ROS Nodes.
