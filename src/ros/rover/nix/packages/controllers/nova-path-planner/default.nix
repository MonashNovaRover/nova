{ lib
, buildRosPackage
, ament-cmake
, control-msgs
, controller-interface
, hardware-interface
, pluginlib
, rclcpp
, rclcpp-lifecycle
, std-srvs
, generate-parameter-library
, rcpputils
, backward-ros
, realtime-tools
, tf2
, tf2-msgs
, geometry-msgs
, tf2-geometry-msgs
, moveit-core
, moveit-ros-planning
, moveit-ros-planning-interface
}:

buildRosPackage {
  name = "nova-path-planner";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova_path_planner-source";
    path = ../../../../controllers/nova_path_planner;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    control-msgs
    controller-interface
    hardware-interface
    pluginlib
    rclcpp
    rclcpp-lifecycle
    std-srvs
    generate-parameter-library
    backward-ros
    realtime-tools
    tf2
    tf2-msgs
    geometry-msgs
    tf2-geometry-msgs
    moveit-core
    moveit-ros-planning
    moveit-ros-planning-interface
  ];
}

