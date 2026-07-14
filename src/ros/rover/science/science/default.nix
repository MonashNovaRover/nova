{ lib
, buildRosPackage
, pythonPackages
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

  nativeBuildInputs = [ ament-cmake ament-cmake-pytest python3Packages.python ];

  buildInputs = [ rclcpp geometry-msgs nav-msgs trajectory-msgs ];

  propagatedBuildInputs = with pythonPackages; [
    rclpy
    jcan
    nova-coms-utils
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
  ];

  doCheck = true;

  preCheck = ''
    export ROS_LOG_DIR="$TMPDIR/.ros/log"
    MOCK_JCAN=$(ls -d ${pythonPackages.mock-jcan}/lib/python*/site-packages 2>/dev/null | head -n 1)
    echo "Injecting mock jcan package into PYTHONPATH for testing..."
    
    if [ -n "$MOCK_JCAN" ]; then
      export PYTHONPATH="$MOCK_JCAN:$PYTHONPATH"
    else
      echo "Error: Could not locate site-packages for mock-jcan."
      exit 1
    fi
  '';

  checkPhase = ''
    runHook preCheck
    ${pythonPackages.pytest}/bin/pytest ../science/pytest
    runHook postCheck
  '';
}
