{ lib
, buildRosPackage
, ament-cmake
, std-msgs
, geometry-msgs
, nav-msgs
, vision-msgs
, visualization-msgs 
, yolo-msgs
}:

buildRosPackage rec {
  name = "nova-utils";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-utils-source";
    path = ../../../nova_utils;
  };
  
  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [ std-msgs nav-msgs ];
  propagatedBuildInputs = [ 
    vision-msgs 
    visualization-msgs 
    geometry-msgs 
    std-msgs 
    yolo-msgs 
  ];
}
