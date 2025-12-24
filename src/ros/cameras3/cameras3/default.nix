{ lib
, buildRosPackage
, ament-cmake
, pkg-config
, rclcpp
, generate-parameter-library
, std-msgs
, std-srvs
, systemd
, nova-cameras3-msgs
}:

buildRosPackage {
  name = "nova-cameras3";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-cameras3-source";
    path = ./.;
  };

  nativeBuildInputs = [ ament-cmake pkg-config ];

  buildInputs = [
    rclcpp
    std-msgs
    std-srvs
    generate-parameter-library
    systemd
    nova-cameras3-msgs
  ];

}
