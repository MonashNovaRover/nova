{ lib
, fetchFromGitHub
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

  src = fetchFromGitHub {
  	owner = "JoelAKruger";
  	repo = "ublox_dgnss";
  	rev = "d6c2fdb";
  	hash = "sha256-cvGmsn3TkV9/HILkPwE+6IBG8N60DFBe6Miafey4WY8=";
  };

  nativeBuildInputs = [ament-cmake rclcpp rclcpp-components rtcm-msgs libcurl-vendor rosidl-default-generators sensor-msgs];
  propagatedBuildInputs = [ std-msgs ];
}
