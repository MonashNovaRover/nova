{ lib
, pkgs
, buildRosPackage
, ament-cmake
, launch
, launch-ros
, nova-yolo-msgs
, nova-yolo-ros
}:

buildRosPackage rec {
  name = "nova-yolo-bringup";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-yolo-bringup";
    path = ../../../nova_yolo_ros/yolo_bringup;
  };
  
  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ launch launch-ros ];

  passthru.workspacePackages = {
    inherit
      nova-yolo-msgs
      nova-yolo-ros;
  };
}
