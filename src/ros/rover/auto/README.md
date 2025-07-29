# Auto
---
This is the main folder for the **Autonomous** subteam!
It consists of our custom ros2 packages which run our stack for the Australian Rover Challenge (ARCh) and University Rover Challenge (URC)

We are currently working on documenting the entire substructure but as of writing (29/07 by Anthony Lew) we have the following:

### auto (./)
- `auto_bringup`
  - Contains our ROS2 bring up folders which are responsible for the initalisation and running of the different nodes/processes.
- `auto_object_localisation`
  - Contains folders relating to object detection and localisation
- `lattice_primitive_generator`
  - Local copy of [Nav2 Lattice Primitives](https://github.com/ros-navigation/navigation2/tree/main/nav2_smac_planner/lattice_primitives)
  - *Yo is this okay to have in our repo? Surely we just use nixfiles or somethin* - **Anthony**
- `nova_auto_interfaces`
  - ROS2 interfaces for the autonomous stack
- `nova_behavior_tree`
  - Our custom bheaviour tree developed using Groot2
- `nova_bt_navigators`
  - behaviour tree integration with Nav2
- `nova_costmap_2d`
  - A custom Nav2 costmap plugin (Last used 2023-2024 design cycle)
- `nova_utils` (formerlly nova_utils)
  - Assorted python scripts for use in the stack
