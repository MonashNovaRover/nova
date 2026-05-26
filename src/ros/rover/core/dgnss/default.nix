{ lib
, buildRosPackage
, pythonPackages
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
  name = "dgnss";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "dgnss-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  buildInputs = [ rclcpp rclpy geometry-msgs nav-msgs trajectory-msgs std-msgs std-srvs nova-interfaces ];
  propagatedBuildInputs = with pythonPackages; [ 
    pyunigps
    smbus2
    wmm-calculator
    pandas 
    folium 
    pymavlink
    pyubx2
  ] ++ [ 
    sensor-msgs
    fabric
    nova-interfaces
  ];
}
