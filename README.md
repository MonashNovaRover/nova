# Autonomous Roadmap for ARC 2022:
--------------------------------------------------

Welcome! This document describes the current roadmap for Autonomous.

We welcome **all feedback and suggestions**. Suggestions can be in the form of a Slack post, message, or a pull request.

--------------------------------------------------
### Stage 0: Research [present - 22/11/2021]

- Look through all the most promising open source code
- Experiment with what can be installed and what can be demoed (e.g. SLAM on rosbag data)
	- None of the open source SLAM packages we found work (or maybe we need to be better at CMake?) 
	- Open3D is great
- Beging Planning Stage 1


-------------------------------------------------

### Stage 1: MVP [22/11/2021 - 18/12/2021]

Build an MVP over a 3 week period starting (officially) 22th November:

The MVP is both a significant step up from the 2021 ARC system, but also not so feature packed that it's unachievable.
For many of the components below, there is relevant code from our ARC 2021 package which can be re-purposed (noting that it will need to be converted to ROS-2 and Python3). 

**MVP Components:**

- Pose System:
	1. Test and tune various pose sources (see Trello for tasks)
	2. Create a pose system utilizing at least two sensors, for example:
	- T265 camera + wheel odometry
	- T265 + AR tag tracking (intending to add the active beacon inputs later on)
	ARC 2021 code references: [T265+wheels=Pose] (https://github.com/novarover/arc_auto/blob/master/scripts/pose/wheel_yaw_pose.py)

- 3D Mapper (non-simulataneous):
	1. Import point-cloud data into Open3D, translate by pose to global coordinates
	2. Build a 3D map with an appropriate representation (e.g. voxel grids)
	Reference: [Open3D](http://www.open3d.org/) 

- Path Planner:
	1. Use 3D information (e.g. height of obstavles) to generate a 2D representation (AKA an occupancy map)
	2. Perform A* and/ or other path planning techniques to plan a path from current location to a goal
	3. Implement the controller which follows the path 
	ARC 2021 reference: 
	- [Path Planning: A*+String Pulling](https://github.com/novarover/arc_auto/blob/master/scripts/path_planning/PathPlanner.py)
	- [Controller (Crow's Algorithm)](https://github.com/novarover/arc_auto/blob/master/scripts/controller/Controller.py)

- Autonomous GUI:
	1. visualise the rover, map, goals and paths all in the one 3D view
	Open3D Viewer: 
	Some links worth browing:
	[Visualising Pointclouds](https://towardsdatascience.com/guide-to-real-time-visualisation-of-massive-3d-point-clouds-in-python-ea6f00241ee0)
	[Open3D Visualisation Examples](http://www.open3d.org/docs/0.9.0/tutorial/Basic/visualization.html)


Note: this system will run on Wombat (the 2021 Rover) but will use ROS-2. Hence, ROS-2 conversion of all other core scripts is also an important part of achieving this MVP.

-----------------------------

### Stage 2: 

Final ARC Autonomous System:

**Boring Stuff:**
- Standardize the ROS network and code interfaces for future development (with a view towards URC-2022 compatibility)
- Document what we have
- Some sort of Autonomous specific design review (tbd).

**Exciting Stuff:**
- Goal detection (incl. active beacons)
- Fine tune components which work well
- Implement RGB-D SLAM if feasible
- Utilizes an EKF to fuse multiple pose sources
- #MachineLearning


Onward! 
