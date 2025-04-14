
# Copyright 2025 Open Source Robotics Foundation
# Distributed under the terms of the BSD license

{ lib, buildRosPackage, fetchurl, ament-cmake, ament-lint-auto, ament-lint-common, rosidl-default-generators }:
buildRosPackage {
  pname = "ros-jazzy-ublox-ubx-interfaces";
  version = "0.5.5-r3";

  src = fetchurl {
    url = "https://github.com/ros2-gbp/ublox_dgnss-release/archive/release/jazzy/ublox_ubx_interfaces/0.5.5-3.tar.gz";
    name = "0.5.5-3.tar.gz";
    sha256 = "sha256-uGJKsL/vscgyl3MqjaXFOx0EEB3AMtadU8wYbsmwB0M=";
  };

  buildType = "ament_cmake";
  buildInputs = [ ament-cmake ];
  checkInputs = [ ament-lint-auto ament-lint-common ];
  propagatedBuildInputs = [ rosidl-default-generators ];
  nativeBuildInputs = [ ament-cmake ];

  meta = {
    description = "UBLOX UBX Interfaces";
    license = with lib.licenses; [ asl20 ];
  };
}