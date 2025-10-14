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
, orocos-kdl
}:

buildRosPackage {
  name = "nova-arm-kinematics";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-arm-kinematics-source";
    path = ./.;
  };

  nativeBuildInputs = [
    ament-cmake
    pkg-config
    ament-cmake-gtest
    ament-lint-auto
  ];

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
    orocos-kdl
  ];

  propagatedBuildInputs = [ 
    pluginlib
    rclcpp
    backward-ros
    tf2
    tf2-msgs
    geometry-msgs
    tf2-geometry-msgs
    eigen
    tf2-eigen
    orocos-kdl
  ];

  doCheck = true;
}
