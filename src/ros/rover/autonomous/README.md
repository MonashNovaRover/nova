## Autonomous - Monash Nova Rover

This is a ROS2 package for Nova Rover's autonomous capabilities

### How to build autonomous
Before running `build autonomous`, you must run the following command: \
`git submodule update --init --recursive`\
Alternatively, clone with: \
`git clone --recurse-submodules git@github.com:MonashNovaRover/rover.git`


#### About the languages we use
This repository contains code written in python and c++, located in the `autonomous` and `src` folders respectively.
The c++ code is for path planning and map building algorithms which we wanted to be as efficient as possible. None of the 
c++ modules interact directly with ROS - i.e. they are not ROS Nodes. Instead, we create python wrappers using Pybind11 and 
call these functions by passing in numpy arrays as arguments, which pybind is able to convert to c++ native types like standard 
vectors. This is somewhat inefficient and pybind is a little bit annoying, so we are looking to move some of these autonomous
nodes entirely into C++ Ros. We also use numpy for a lot of point-cloud processing and various map building operations, 
as well as for storing python-local copies of our 2D occupancy grid. 
## Note to future code explorers:
A *lot* of this code was written before the team had a complete understanding of Ros2 package structures, parameters and other
features. As a result, there are many many instances of poor practices which we are in the process of correcting. Take what you
see here with a grain of salt. It's a case of do as I say, not as I do...

#### Folder Structure:
The root python folder `autonomous` is divided into sub-folders containing different aspects of the rover's code:
- `planning`: path planning interface to A* c++ code via Pybind11, as well as high level `goal_selector.py` which plans
  out the route taken by the rover.
- `controller`: controller state machine that directs the rover's movements to follow a planned path (set of waypoints)
- `mapping`: `grid_2d.py` a wrapper for the numpy array which stores our occupancy grid in python-land, and `mapper.py`, which
  takes in pointclouds and passes them off to c++-land to be subjected to mapping algorithms. `map.png` for our high-tech simulated
  occupancy grid.
- `localisation`: `pose_converter.py` takes input poses from our sensors and converts to our coordinate system, then publishes
  the tf2-transforms with the rover's position
- `cameras`: direct python interfaces to the depth and tracking cameras, as well as AR tracking code 
- `config`: `ros_config.py` lists key topic names and types, `runtime_params.py` has many constants we like to tweak
- `math_utils`: pure functions used in other modules
- `vis`: non functional functions for converting data into visualizable ros topics, like the rover as a point-cloud
