{ lib
, writeShellApplication
, buildRosPackage
, ament-cmake
, rclcpp
, std-srvs
, geometry-msgs
, nav-msgs
, sensor-msgs
, eigen
, orocos-kdl
, systemd
, python3Packages
, libcanmd
, nova-arm-interfaces
, nova-input-interfaces
, nova-inputs
, nova-interfaces
, SDL2
}:

buildRosPackage {
  name = "arm";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "arm-source";
    path = ./.;
    filter = lib.novaSourceFilter [
    ]
      path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    std-srvs
    geometry-msgs
    nav-msgs
    sensor-msgs
    eigen
    orocos-kdl
    systemd
    libcanmd
    nova-arm-interfaces
    nova-input-interfaces
    nova-inputs
    nova-interfaces
    SDL2
    SDL2.dev
  ];

  propagatedBuildInputs = [
    python3Packages.scipy
  ];
}
