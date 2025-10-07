# Controllers

## Overview

Controllers are classes that interface with the cards on the CAN bus.
They construct the required CAN message based on the corresponding Control's state.

Each Controller has a callback function that is called periodically (default is 0.1 seconds) by a Controller Node.
Within this callback a jcan frame is constructed via an abstract method that all Controllers must implement.
This frame is sent on the CAN bus periodically unless it's configured to only send messages when the message changes.

## Example

Positional Servos are a good example to demonstrate.

In the 24/25 design cycle the can command to position the base of the science gimbal cam was:

- `0A0#04XX` where XX was the desired position as a percentage (ie 00 = 0 deg, FF = 360 deg)

The JonoPositionController had a corresponding OneAxisPositionControl which kept track of what
angle the servo should be at (eg 80 deg or 240 deg).
The job of the controller is to translate this into a CAN message, and then send it on the CAN bus.

The Controller Node provides the CAN ID (`0A0`, and the position command (`04`) and the controller
translates the current position in degrees to CAN format.

eg. 90 deg -> `0A0#0444`

Similar logic applies to other controllers such as CMDs with direction and velocity.
