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
, nova-arm-kinematics
}:

buildRosPackage {
  name = "nova-twistmapper";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova_twistmapper-source";
    path = ./.;
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
    nova-arm-kinematics
  ];
}

