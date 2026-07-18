# science_interfaces

Custom ROS2 interfaces for science.

## Adding Interfaces

Interface definiations should be added in the corresponding folder:
- [./msg](./msg) for Messages
- [./srv](./srv) for Services
- [./action](./action) for Actions

The interface file name will then need to be added to the [`CMakeLists.txt`](./CMakeLists.txt) in the 
appropriate labeled section.

## Dependencies

Where possible try to reuse standard message types within your interfaces.
These are some libraries that we commonly reference:
- [`std_msgs`](https://docs.ros.org/en/ros2_packages/rolling/api/std_msgs/) 
  - Please do not use the primitive type messages (e.g. `Bool`, `Int8`, ...)
  - Use `std_msgs/Header` or `std_msgs/ColorRGBA`.
- [`geometry_msgs`](https://docs.ros2.org/foxy/api/geometry_msgs/index-msg.html)
- [`sensor_msgs`](https://docs.ros.org/en/ros2_packages/humble/api/sensor_msgs/)
- [`nav_msgs`](https://docs.ros.org/en/ros2_packages/rolling/api/nav_msgs/)

### How to add a dependency

1. Check that the dependency isn't already imported in [`default.nix`](./default.nix) and [`CMakeLists.txt`](./CMakeLists.txt).
2. Add the dependency to [`default.nix`](./default.nix).
    - The dependency will have a name in `kabab-case`.
    - You will need to add it at the top inports statement and in the `propagatedBuildInputs`.
3. Add the dependency to [`CMakeLists.txt`](./CMakeLists.txt).
   - The dependency will have a name in `snake_case`.
   - You will need to add it at the top in the `find_package` section and near the end on the line starting with `DEPENDENCIES`.
