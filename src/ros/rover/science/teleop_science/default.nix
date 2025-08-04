# You only need to use this file if you use nix to build your packages.
# If you don't, feel free to delete this.

{ lib
, pkg-config
, buildRosPackage
, ament-cmake
, turtlesim
, joy
, teleop-modular
, teleop-modular-node
, teleop-modular-twist
, teleop-modular-joy
}:

buildRosPackage {
  name = "teleop-science";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "teleop-science-source";
    path = ./.;
  };

  nativeBuildInputs = [
    ament-cmake
    pkg-config
  ];

  buildInputs = [
  ];

  propagatedBuildInputs = [
    turtlesim
    joy
    teleop-modular
    teleop-modular-node
    teleop-modular-twist
    teleop-modular-joy
  ];
}
