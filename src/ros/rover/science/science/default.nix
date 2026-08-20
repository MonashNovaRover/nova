{ lib
, buildRosPackage
, ament-cmake
, ament-cmake-pytest
, python3Packages
, rclcpp
, rclpy
, geometry-msgs
, nav-msgs
, trajectory-msgs
, nova-science-interfaces
, nova-input-interfaces
, nova-python-control
, nova-camera-msgs
, nova-python-control2
, teleop-modular-python-utils
}:

buildRosPackage {
  name = "science";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "science-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [ rclcpp geometry-msgs nav-msgs trajectory-msgs ];

  propagatedBuildInputs = lib.optional (python3Packages ? nova-coms-utils) python3Packages.nova-coms-utils
    ++ with python3Packages; [
    rclpy
    jcan
    pymodbus
    gphoto2
    opencv4
    pyserial
    python3Packages.minimalmodbus
    nova-python-control
    nova-python-control2
    nova-input-interfaces
    nova-camera-msgs
    teleop-modular-python-utils
    nova-science-interfaces
    nova-pytest-framework
  ];

  doCheck = true;

  checkPhase = ''
    runHook preCheck
    ${python3Packages.pytest}/bin/pytest ../science/tests
    runHook postCheck
  '';
}
