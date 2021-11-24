# CONTROL :video_game:
Scripts located in this ROS 2 package control any of the arm, driver or other controller scripts. They interface the joysticks and gamepads with the custom motor driver (CMD) data and are able to send signals to the electrical systems. All of the scripts in this package are written in C++ and are found in the `src` folder and relevant subdirectories. Note that not every class here interfaces with ROS, but some are purely helper classes (such as the Joystick and Wheels for example).

## Installation :hammer:
The following steps are required to ensure all of the scripts run correct on any Linux Ubuntu 18.04 device. Please follow the installation instructions if you are facing difficulty running scripts.

```
sudo apt install libudev-dev -y
```

## Script Directory :clipboard:

- **Drive Publisher**: Publishes drive data by analysing the input and sending the required steer and RPM values to the drive node for analysing. This script should be run on the *base station*.
    `ros2 run control drive_cmd`
<br>

- **Drive Subscriber**: Subscribes to the wheel commands and interfaces with the wheel class to send CAN signals to the wheels. This script does not require CAN to run, but creates instances of wheel classes. This script should be run on the *rover*.
    `ros2 run control driver`
<br>

- **Input Publisher**: Publishes input data from all of the currently plugged in joysticks over the ROS networks. By default, the Xbox controller will always publish correctly when plugged in, but the order of the Thrustmaster joysticks matter, and could be incorrectly ordered. This script should be run on the *base station*.
    `ros2 run control inputs`
<br>

- **Joystick**: Not a ROS script, but interfaces with the Gamepad library and stores data for each of the inputs and joystick actions. This script cannot be executed by ROS, but scripts may include this class and create instances of it (as done in the input publisher class).
<br>

- **Joystick Gamepad**: Inherits from the Joystick script and is able to store messages from a gamepad such as an Xbox controller device, primarily used to drive the rover.
<br>

- **Joystick Thrustmaster**: Inherits from the Joystick script and is able to store messages from the Thrustmaster joystick devices, primarily used to move the arm.
<br>

- **Wheel**: Not a ROS script, but interfaces with the CAN classes and is able to communicate with the wheel CMDs and spin the wheels in certain directions.
<br>
