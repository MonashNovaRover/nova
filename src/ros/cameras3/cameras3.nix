{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, generate-parameter-library
, std-msgs
}:

buildRosPackage {
  name = "nova-cameras3";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-cameras3-source";
    path = ./.;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    std-msgs
    generate-parameter-library
  ];

}
