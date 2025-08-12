{ lib
, buildRosPackage
, pythonPackages
, ament-cmake
, python3Packages
, rclcpp
, rclpy
, geometry-msgs
, nav-msgs
, trajectory-msgs
, nova-interfaces
, nova-input-interfaces
, nova-python-control
, nova-camera-msgs
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

  buildInputs = [ rclcpp rclpy geometry-msgs nav-msgs trajectory-msgs nova-interfaces ];

  propagatedBuildInputs = with pythonPackages; [
    jcan
    nova-coms-utils
    pymodbus
    gphoto2
    opencv4
    pyserial
    python3Packages.minimalmodbus
  ] ++
  [
    nova-python-control
    nova-input-interfaces
    nova-camera-msgs
    teleop-modular-python-utils
  ];
}

/*
nix-shell -p 'with import /home/nova/nova/nixfiles { }; pkgs.ros.nova-workspace.override {
	novaPackages = {
		inherit (pkgs.ros)
		nova-science
	};
}'
*/