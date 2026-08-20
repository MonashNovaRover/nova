{ lib
, buildRosPackage
, python3Packages
, ament-cmake
, rclcpp
, rclpy
, geometry-msgs
, nav-msgs
, trajectory-msgs
, std-msgs
, std-srvs
, nova-interfaces
, sensor-msgs
}:

buildRosPackage {
  name = "electronics";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "electronics-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [ rclcpp rclpy geometry-msgs nav-msgs trajectory-msgs std-msgs std-srvs nova-interfaces ];
  propagatedBuildInputs = with python3Packages; [ 
    smbus2 
    wmm-calculator
    pandas 
    folium 
    pymavlink
  ] ++ [ 
    sensor-msgs
    fabric
    nova-interfaces
  ];
}
