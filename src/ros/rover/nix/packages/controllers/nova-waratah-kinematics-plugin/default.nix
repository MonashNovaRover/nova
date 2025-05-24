{ lib
, buildRosPackage
, ament-cmake
, pluginlib
, rclcpp
, rcpputils
, backward-ros
, tf2
, tf2-msgs
, geometry-msgs
, tf2-geometry-msgs
, eigen
, tf2-eigen
, moveit-core
}:

buildRosPackage {
  name = "nova-waratah-kinematics-plugin";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "waratah_kinematics_plugin-source";
    path = ../../../../controllers/waratah_kinematics_plugin;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    pluginlib
    rclcpp
    backward-ros
    tf2
    tf2-msgs
    geometry-msgs
    tf2-geometry-msgs
    eigen
    tf2-eigen
    moveit-core
  ];
}
