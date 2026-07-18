{ lib
, buildRosPackage
, ament-cmake
, rclcpp
, nova-input-interfaces
, systemd
, SDL2
}:

buildRosPackage {
  name = "inputs";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "inputs-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ]
      path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    rclcpp
    nova-input-interfaces
    systemd
    SDL2
    SDL2.dev
  ];
}
