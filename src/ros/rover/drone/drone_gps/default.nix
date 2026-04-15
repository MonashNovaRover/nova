{
  pkgs,
  buildRosPackage,
  ament-cmake,
}:

buildRosPackage {
  name = "drone-gps";
  buildType = "ament_cmake";

  src = builtins.path {
    name = "drone-gps-source";
    path = ./.;
  };

  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = with pkgs; [
    mavros
  ];
}
