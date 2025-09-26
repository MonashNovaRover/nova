{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, opencv4
, nav2-costmap-2d
, pluginlib
, rosidl-default-generators
, launch
, launch-ros
}:

buildRosPackage {
  name = "costmap-2d";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-costmap-2d-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  buildInputs = [ pluginlib opencv4 rclcpp nav2-costmap-2d ];
  propagatedBuildInputs = [ launch launch-ros ];
}
