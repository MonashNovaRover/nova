# Sensors

## Overview

Sensors simply listen to the CAN bus for a specified CAN id and just keep track of the latest message.

There are a number of types of sensors that just listen for different things, 
e.g. IntegerSensors translate CAN message to integers and ToggleCommandSensors
are just on or off.

Sensors also have the potential to publish the received data straight to a topic,
however this functionality remains unimplemented.

## Example

A super common example in science is environmental sensors.

For example in the 24/25 design cycle we had many sensors for temperature, humidity, pressure etc.
The electrical boards in the payload interfaced with the sensor equipment and published the readings
onto the CAN bus which were listen to and published via sensors.