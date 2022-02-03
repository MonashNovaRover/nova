## Monash Nova Rover Autonomous

This is a ROS-2 (Eloquent) package for Nova Rover's autonomous capabilities. All code was developed by Nova Rover unless otherwise stated.

The broad capabilities provided are: 

#### Efficient 3D mapping into voxel grids
- Mapping is local to past pose information, therefore the purpose is to allow for path planning and obstacle avoidance (we do not perform simultaneous localisation and mapping)
- Point-clouds from the Intel Realsense D415 OR D435 Depth Camera are translated based on the Rover's pose and plotted into a discrete 3D map
- The map can be visualised in RVIZ as a point-cloud, making for easy communication to a base station over ROS
- Future improvements inlcude:
    - Rolling map for un-bounded spaces
    - Regular cleaning out of stale obstacles due to pose drift

#### 3D Path Planning
The purpose of our path planning algorithms is to:
- Avoid impassible obstacles with a predefined safety distance
- Avoid hills where possible - i.e. attempt to drive through valleys while still
- Balance the avoiding of hills with finding the "shortest" path
- Stay within a pre-defined competition zone

Weighted A* forms the basis for a more advanced set of path planning tools current being developed and experimented with, which include:
- Searching through cross sections (searching through horizontal slices of the 3D map)
- Radius based avoidance (identifying key corner points and avoiding them with certain distances)
- String pulling (converting Manhattan style A* paths into paths with fewer straight line segments)
- GPU accelerated heatmaps (coming soon)
    - Utilizing the NVIDIA Jetson's graphics processor through CUDA to perform convolution operations on the 3D map, thus producing a 2D heatmap useful for informing the A* branching function, A* heuristic function
    
    
#### Optional pose sources
The package aims to be flexible to allow for a the following sensors:
- Differential GPS (URC only)
- Compass (URC only)
- Intel Realsense T265 Tracking Camera
- Intel Realsense D415 OR D435 Depth Camera
- Wheel based velocity and wheel encoder feedback

#### AR Tag Tracking and Search
In order to find certain goals, the high level competition specific control algorithms path plan towards approximate destinations and search for known artificial landmarks, then continually re-plan until they have been approached.
