## Autonomous - Monash Nova Rover

This is a ROS-2 (Eloquent) package for Nova Rover's autonomous capabilities

#### About the languages we use
This repository contains code written in python and c++, located in the `autonomous` and `src` folders respectively.
The c++ code is for path planning and map building algorithms which we wanted to be as efficient as possible. None of the 
c++ modules interact directly with ROS - i.e. they are not ROS Nodes. Instead, we create python wrappers using Pybind11 and 
call these functions by passing in numpy arrays as arguments, which pybind is able to convert to c++ native types like standard 
vectors. We also use numpy for a lot of point-cloud processing and various map building operations, as well as for storing 
python-local copies of our 2D occupancy grid. 

#### Folder Structure:
The root python folder `autonomous` contains two key files: `main.py`, and `update_goals.py`, both of which are run in seperate 
terminals during competition. The rest of the code is in sub folders which are installed as python packages, namely:
- `planning`: path planning interface to c++ code via Pybind11
- `controller`: control algorithms such as "yaw-star", search, and all competition specific logic
- `mapping`: for conversion of 3D data into 2D occupancy maps via plane fitting and heigh difference mapping
- `localisation`: pose conversion, an Extended Kalman Filter for pose estimation, and other tools
- `cameras`: direct python interfaces to the depth and tracking cameras, as well as AR tracking code 
- `config`: `ros_config.py` lists key topic names and types, `runtime_params.py` has many constants we like to tweak
- `math_utils`: pure functions used in other modules
- `vis`: non functional functions for converting data into visualizable ros topics, like the rover as a point-cloud
- `resources`: misc files like saved numpy arrays for various things

#### Optional pose sources
The package aims to be flexible to allow for a the following sensors:
- Differential GPS (URC only)
- Compass (URC only)
- Intel Realsense T265 Tracking Camera
- Intel Realsense D415 OR D435 Depth Camera
- Wheel based velocity and wheel encoder feedback
