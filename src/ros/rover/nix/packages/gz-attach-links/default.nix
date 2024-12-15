{ buildRosPackage
, ament-cmake
, launch
, launch-ros
, ros-gz
, simple-launch
, slider-publisher
}:

buildRosPackage rec {
  name = "gz-attach-links";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "gz-attach-links-source";
    path = ../../../gz_attach_links;
  };
  
  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ 
    launch 
    launch-ros 
    ros-gz
    simple-launch
    slider-publisher
  ];
}
