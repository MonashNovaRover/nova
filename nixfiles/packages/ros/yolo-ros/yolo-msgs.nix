{ buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
, geometry-msgs
, fetchFromGitHub
}:

buildRosPackage rec {
  pname = "yolo-msgs";
  buildType = "ament_cmake";

  src = fetchFromGitHub {
    owner = "mgonzs13";
    repo = "yolo_ros";
    rev = "5bca9341e8da3f5c99cb9edbb747fda7ddfe78fb";
    hash = "";
  };

  sourceRoot = "${src.name}/yolo_ros";

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  buildInputs = [std-msgs geometry-msgs];
}
