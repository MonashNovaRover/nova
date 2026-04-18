{
  buildRosPackage,
  ament-cmake,
  pythonPackages,
  launch,
  launch-ros,
  nova-interfaces
}:

buildRosPackage {
  name = "drone-gps";

  src = builtins.path {
    name = "drone-gps-source";
    path = ./.;
  };

  buildType = "ament_cmake_python";

  nativeBuildInputs = [ ament-cmake ];
  
  propagatedBuildInputs = [
    launch
    launch-ros
    nova-interfaces
  ] ++ (with pythonPackages; [
    pymavlink
  ]);
}
