{ lib
, writeShellApplication
, buildRosPackage
, ament-cmake
, rclcpp
, rclpy
, std-srvs
, geometry-msgs
, nav-msgs
, trajectory-msgs
, sensor-msgs
, eigen
, orocos-kdl
, systemd
, pythonPackages
, nova-core
}:

buildRosPackage {
  name = "control";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "control-source";
    path = ../../../control;
    filter = lib.novaSourceFilter [
      "!include/jcan_*/libjcan.a"
    ]
      path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    rclpy
    std-srvs
    geometry-msgs
    nav-msgs
    trajectory-msgs
    sensor-msgs
    eigen
    orocos-kdl
    systemd
    nova-core
  ];

  propagatedBuildInputs = with pythonPackages; [
    jcan
  ];

  postInstall = ''
    mkdir -p "$out/bin"
    ln -s ${writeShellApplication {
      name = "blcmd_disable";
      text = builtins.readFile ./macros/blcmd_disable.sh;
    }}/bin/* "$out/bin"
  '';
}
