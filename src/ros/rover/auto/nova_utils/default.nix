{
  buildRosPackage,
  ament-cmake,
  std-msgs,
  geometry-msgs,
  sensor-msgs,
  sensor-msgs-py,
  geographic-msgs,
  robot-localization,
  nav-msgs,
  vision-msgs,
  cv-bridge,
  visualization-msgs,
  yolo-msgs,
  tf2-ros,
  tf-transformations,
  rerun,
  pythonPackages,
}:

buildRosPackage rec {
  name = "nova-utils";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-utils-source";
    path = ./.;
  };

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [
    std-msgs
    nav-msgs
  ];
  propagatedBuildInputs = with pythonPackages; [
    vision-msgs
    visualization-msgs
    geometry-msgs
    geographic-msgs
    robot-localization
    std-msgs
    yolo-msgs
    tf2-ros
    tf-transformations
    rerun
    cv-bridge
    sensor-msgs
    sensor-msgs-py
    pythonPackages.geographiclib
    pythonPackages.rerun-sdk
  ];
}
