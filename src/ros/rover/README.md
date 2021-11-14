# CONTROL :video_game:
Scripts located in this ROS 2 package control any of the arm, driver or other controller scripts. They interface the joysticks and gamepads with the custom motor driver data and are able to send signals to the electrical systems. All of the scripts in this package are written in C++ and are found in the `source` folder.

## Installation
The following steps are required to ensure all of the scripts run correct on any Linux Ubuntu 18.04 device. Please follow the installation instructions if you are facing difficulty running scripts.

```
sudo apt install libudev-dev -y
```

## Script Directory

- **Drive Publisher**: Publishes drive data by analysing the input and sending the required steer and RPM values to the drive node for analysing.
    `ros2 run control drive_pub`
<br>

- **Input Publisher**: Publishes input data from all of the currently plugged in joysticks over the ROS networks.
    `ros2 run control inputs`
<br>

- **Joystick**: Not a ROS script, but interfaces with the Gamepad library and stores data for each of the inputs and joystick actions. This script cannot be executed by ROS, but scripts may include this class and create instances of it (as done in the input publisher class).