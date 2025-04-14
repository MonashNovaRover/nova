
# Copyright 2025 Open Source Robotics Foundation
# Distributed under the terms of the BSD license

{ lib, 
  buildRosPackage, 
  fetchurl, 
  ament-cmake, 
  ntrip-client-node, 
  ublox-dgnss-node, 
  ublox-nav-sat-fix-hp-node, 
  ublox-ubx-interfaces, 
  ublox-ubx-msgs 
}:

buildRosPackage rec {
  name = "ros-jazzy-ublox-dgnss";
  buildType = "ament_cmake";
  version = "0.5.5-r3";

  src = fetchurl {
    url = "https://github.com/ros2-gbp/ublox_dgnss-release/archive/release/jazzy/ublox_dgnss/0.5.5-3.tar.gz";
    name = "0.5.5-3.tar.gz";
    sha256 = "sha256-mzmcFu6UvnE/q096viexA0YgQNHD5A9ZgCVEFdEQMOg=";
  };

  buildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ ntrip-client-node ublox-dgnss-node ublox-nav-sat-fix-hp-node ublox-ubx-interfaces ublox-ubx-msgs ];
  nativeBuildInputs = [ ament-cmake ];
}