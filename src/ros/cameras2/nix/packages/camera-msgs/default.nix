{ cmake, }:
{ buildEnv, buildRosPackage, ament-cmake, rosidl-default-generators, }:

buildRosPackage {
  pname = "ros-camera-msgs";
  version = "git";
  buildType = "ament_cmake";

  src = ../../../camera_msgs;

  nativeBuildInputs = [ cmake ament-cmake rosidl-default-generators ];
}
