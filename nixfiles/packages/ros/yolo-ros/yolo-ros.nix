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
, ultralytics
, super-gradients
, lap
}:

buildRosPackage rec
{
  pname = "yolo-ros";
  version = "4.0.1";
  buildType = "ament_python";

  src = fetchFromGitHub {
    owner = "mgonzs13";
    repo = "yolo_ros";
    rev = "5bca9341e8da3f5c99cb9edbb747fda7ddfe78fb";
    hash = "";
  };

  sourceRoot = "${src.name}/yolo_ros";

  propagatedBuildInputs = [
    rclpy
    python3Packages.typing-extensions
    python3Packages.pytorch
    python3Packages.numpy
    opencv4
    ultralytics
    super-gradients
    lap
    cv-bridge
    sensor-msgs
    visualization-msgs
    std-msgs
    std-srvs
    message-filters
    yolo-msgs
  ];

  # The package name is just "opencv", not "opencv-python".
  # https://discourse.nixos.org/t/how-to-give-opencv-dependency-to-python-package/16949
  preBuild = ''
    sed -i 's/opencv-python/opencv/g' pyproject.toml
  '';
}
