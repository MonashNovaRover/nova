{ lib
, buildRosPackage
, ament-cmake
, hardware-interface
, rclcpp-lifecycle
, rclcpp
, eigen
}:

buildRosPackage {
  name = "nova-controller-common";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova_controller_common-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    hardware-interface
    rclcpp-lifecycle
    rclcpp
    eigen
  ];
}