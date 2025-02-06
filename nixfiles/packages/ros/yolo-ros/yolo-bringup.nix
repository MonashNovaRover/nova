{ buildRosPackage
, ament-cmake
, launch
, launch-ros
, yolo-msgs
, yolo-ros
, fetchFromGitHub
}:

buildRosPackage rec {
  name = "yolo-bringup";
  buildType = "ament_cmake";

  src = fetchFromGitHub {
    owner = "mgonzs13";
    repo = "yolo_ros";
    rev = "fa4c774294c915dcdc31e7359c2b887a0c30221a";
    hash = "sha256-BH+orBLTaqkPCx42/liMJyPF6IOWzeLiL8AsiyuvtCI=";
  };

  sourceRoot = "${src.name}/yolo_bringup";
  
  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ launch launch-ros ];

  passthru.workspacePackages = {
    inherit
      yolo-msgs
      yolo-ros;
  };
}
