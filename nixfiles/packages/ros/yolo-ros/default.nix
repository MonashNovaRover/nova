{ 
  buildRosPackage, 
  cv-bridge, 
  fetchFromGitHub, 
  message-filters, 
  opencv4, 
  python3Packages, 
  rclpy, 
  sensor-msgs, 
  std-msgs, 
  std-srvs, 
  visualization-msgs, 
  yolo-msgs, 
}:

buildRosPackage rec {
  name = "yolo-ros";
  version = "4.7.1";
  buildType = "ament_python";

  src = fetchFromGitHub {
    owner = "mgonzs13";
    repo = "yolo_ros";
    rev = version;
    hash = "sha256-SZaG+IEINVn9lR7U5JrUYLpJdNHOKXH0jq1k3xjUB9k=";
  };

  sourceRoot = "${src.name}/yolo_ros";

  buildInputs = [
    cv-bridge
    message-filters
    opencv4
    rclpy
    sensor-msgs
    std-msgs
    std-srvs
    visualization-msgs
    yolo-msgs
  ] ++ (with python3Packages; [
    lap
    numpy
    super-gradients
    torch
    typing-extensions
    ultralytics
  ]);
}
