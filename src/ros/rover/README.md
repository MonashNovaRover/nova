<p align="center">
  <a href="https://www.novarover.space/">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://images.squarespace-cdn.com/content/5d907deadfb23123fec64602/831da8ca-c17d-49cc-b325-0f7acbe62b2d/Monash+Nova+Rover+white+text+transparent.png?format=1500w&content-type=image%2Fpng">
      <img src="https://images.squarespace-cdn.com/content/5d907deadfb23123fec64602/831da8ca-c17d-49cc-b325-0f7acbe62b2d/Monash+Nova+Rover+white+text+transparent.png?format=1500w&content-type=image%2Fpng" width="500px" alt="Monash Nova Rover logo">
    </picture>
  </a>
</p>

# Rover

`rover` is the set of mostly ROS2 packages that operate on-rover for the [Monash Nova Rover](https://www.novarover.space/) student team. 

## Project Structure

| Folder | Description |
|--------|-------------|
| [`arm`](./arm) | Robotic arm control code. |
| [`auto`](./auto) | Autonomous stack. |
| [`chassis`](./chassis) | Code for systems in the chassis, including LEDs, GPS etc. |
| [`docs`](./docs) |  |
| [`drive`](./drive) | Drive control code. |
| [`excavation_construction`](./excavation_construction) | Code related to the ARC Excavation and Construction payload. |
| [`hardware_interfaces`](./hardware_interfaces) | Hardware interfaces, including for ROS2 Control. |
| [`nova_bringup`](./nova_bringup) | Launch files. |
| [`nova_generic`](./nova_generic) | Generic packages that can be widely used. |
| [`nova_interfaces`](./nova_interfaces) | [DEPRECATED] Central location for interfaces used across the repo. |
| [`old_inputs`](./old_inputs) | Old input system - we are now moving to [`teleop_modular`](https://github.com/BaileyChessum/teleop_modular) |
| [`rover_description`](./rover_description) | URDFs |
| [`science`](./science) | Code related to the ARC and URC Science payloads and tasks. |
| [`simulations`](./simulations) | Simulations including Gazebo. |

## Running ROS2 Nodes

ROS 2 packages in this repository provide both individual nodes and launch files for starting multiple nodes together.

### Running Nodes Directly

You can run a single node with:

```sh
ros2 run <package_name> <node_executable>
```

This is useful for testing or running components individually.

### Running with Launch Files

Launch files are provided to start groups of nodes with their required configuration. Use:

```sh
ros2 launch <package_name> <launch_file>.launch.py
```

All launch files are organized in the \<package>_bringup folders.
