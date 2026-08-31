{
  buildRosPackage,
  ament-cmake,
  rclpy,
  rclcpp,
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
  tf2-msgs,
  tf-transformations,
  rerun,
  python3Packages,
  nova-interfaces,
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
    rclpy
    rclcpp
    std-msgs
    nav-msgs
  ];
  propagatedBuildInputs = [
    vision-msgs
    visualization-msgs
    geometry-msgs
    geographic-msgs
    robot-localization
    std-msgs
    yolo-msgs
    tf2-ros
    tf2-msgs
    tf-transformations
    rerun
    cv-bridge
    sensor-msgs
    sensor-msgs-py
    python3Packages.geographiclib
    python3Packages.rerun-sdk
    python3Packages.rich
    nova-interfaces
  ];
}
