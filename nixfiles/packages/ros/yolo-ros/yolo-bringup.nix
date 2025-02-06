{ buildRosPackage
, ament-cmake
, launch
, launch-ros
, yolo-msgs
, yolo-ros
, fetchFromGitHub
}:

buildRosPackage rec {
  pname = "yolo-bringup";
  buildType = "ament_cmake";

  src = fetchFromGitHub {
    owner = "mgonzs13";
    repo = "yolo_ros";
    rev = "5bca9341e8da3f5c99cb9edbb747fda7ddfe78fb";
    hash = "";
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
