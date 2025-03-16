{ buildRosPackage
, ament-cmake
, rosidl-default-generators
, std-msgs
, geometry-msgs
, fetchFromGitHub
}:

buildRosPackage rec {
  name = "yolo-msgs";
  buildType = "ament_cmake";

  src = fetchFromGitHub {
    owner = "mgonzs13";
    repo = "yolo_ros";
    rev = "fa4c774294c915dcdc31e7359c2b887a0c30221a";
    hash = "sha256-BH+orBLTaqkPCx42/liMJyPF6IOWzeLiL8AsiyuvtCI=";
  };

  sourceRoot = "${src.name}/yolo_msgs";

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  buildInputs = [std-msgs geometry-msgs];
}
