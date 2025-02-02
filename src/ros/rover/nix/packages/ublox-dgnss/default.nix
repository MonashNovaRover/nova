{ lib
, fetchgit
, buildRosPackage
, ament-cmake
, std-msgs
, rclcpp
, rclcpp-components
, rtcm-msgs
, libcurl-vendor
, rosidl-default-generators
, sensor-msgs
}:

buildRosPackage {
  name = "ublox-dgnss-custom";
  buildType = "ament_cmake";

  src = fetchgit {
  	url = "https://github.com/JoelAKruger/ublox_dgnss.git";
  	sparseCheckout = ["ublox_ubx_msgs"];
  	rev = "d6c2fdb";
  	hash = "sha256-4+znWCmibeKvB2HcBiGusHpVaqE22rmxxZG6LCVok1k=";
  };

  nativeBuildInputs = [ament-cmake rclcpp rclcpp-components rtcm-msgs libcurl-vendor rosidl-default-generators sensor-msgs];
  propagatedBuildInputs = [ std-msgs ];
}
