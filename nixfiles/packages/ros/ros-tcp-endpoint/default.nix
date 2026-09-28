{ fetchFromGitHub, 
  buildRosPackage, 
  rclpy
}:

buildRosPackage rec
{
  name = "ros-tcp-endpoint";
  version = "0.7.0";
  buildType = "ament_python";

  src = fetchFromGitHub {
    owner = "Unity-Technologies";
    repo = "ROS-TCP-Endpoint";
    rev = "54c1a64b6d5ef6ffa0a0431570bb74329b79b15b";
    hash = "sha256-ozVS1P+wVAXjysE5jS8JB4CT9QEtr3DiC0jHfKN/+9c=";
  };

  propagatedBuildInputs = [
    rclpy
  ];
}
