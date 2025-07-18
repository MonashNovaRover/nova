# Controls

## Overview

Controls provide high level logic and state for a specified component.
Controls are used within a Controller Node to update state when specific inputs are received.
Controllers then access the current state within a control and use that to construct CAN frames
to send to boards.

Each control has different state and methods, and apply to different situations.

## Example

A common example is the use of the OneAxisVelocityControl. 
This class is used for science augers, analysis arms and anything that has a motor that runs in a direction at a velocity.

For science, the joysticks are used to control the auger moving up and down via a motor.
Depending on the directed and how far the joysticks are pressed determines the direction 
and speed of the auger actuating.

These variables are updated within the OneAxisVelocityControl.
The corresponding VelocityController then uses the current state to construct accurate CAN
message to change the speed and direction of the motor.
