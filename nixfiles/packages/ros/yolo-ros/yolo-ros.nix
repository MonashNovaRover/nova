{ fetchFromGitHub
, buildRosPackage
, python3Packages
, opencv4
, sensor-msgs
, visualization-msgs
, rclpy
, std-msgs
, cv-bridge
, std-srvs
, message-filters
, yolo-msgs
}:

buildRosPackage rec
{
  name = "yolo-ros";
  version = "4.0.1";
  buildType = "ament_python";

  src = fetchFromGitHub {
    owner = "mgonzs13";
    repo = "yolo_ros";
    rev = "fa4c774294c915dcdc31e7359c2b887a0c30221a";
    hash = "sha256-BH+orBLTaqkPCx42/liMJyPF6IOWzeLiL8AsiyuvtCI=";
  };

  sourceRoot = "${src.name}/yolo_ros";

  propagatedBuildInputs = [
    rclpy
    python3Packages.typing-extensions
    python3Packages.pytorch
    python3Packages.numpy
    python3Packages.ultralytics
    python3Packages.super-gradients
    python3Packages.lap
    opencv4
    cv-bridge
    sensor-msgs
    visualization-msgs
    std-msgs
    std-srvs
    message-filters
    yolo-msgs
  ];

  # UNNEEDED DUE TO sourceRoot
  # The package name is just "opencv", not "opencv-python".
  # https://discourse.nixos.org/t/how-to-give-opencv-dependency-to-python-package/16949
  #preBuild = ''
  #  sed -i 's/opencv-python/opencv/g' requirements.txt
  #'';
}
