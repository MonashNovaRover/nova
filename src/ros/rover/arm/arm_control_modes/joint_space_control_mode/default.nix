{ lib
, pkg-config
, buildRosPackage
, ament-cmake
, ament-cmake-gtest
, ament-lint-auto
, gtest
, rclcpp
, geometry-msgs
, generate-parameter-library
, pluginlib
, std-msgs
, teleop-modular-control-mode
, std-srvs
, nova-interfaces
}:

buildRosPackage {
  name = "nova-joint-space-control-mode";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-joint-space-control-mode-source";
    path = ./.;
  };

  nativeBuildInputs = [
    ament-cmake
    pkg-config
    ament-cmake-gtest
    ament-lint-auto
    gtest
  ];

  buildInputs = [
    rclcpp
    std-msgs
    std-srvs
    generate-parameter-library
    geometry-msgs
    pluginlib
    teleop-modular-control-mode
    nova-interfaces
  ];

  propagatedBuildInputs = [ geometry-msgs ];

  # Enable running tests during build
  doCheck = true;
}
