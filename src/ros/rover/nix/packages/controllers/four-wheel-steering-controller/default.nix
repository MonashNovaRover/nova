{ lib
, buildRosPackage
, ament-cmake
, control-msgs
, controller-interface
, hardware-interface
, pluginlib
, rclcpp
, rclcpp-lifecycle
, std-srvs
, steering-controllers-library
, generate-parameter-library
, rcpputils
, admittance-controller
}:

buildRosPackage {
  name = "four-wheel-steering-controller";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "four_wheel_steering_controller-source";
    path = ../../../../controllers/ros2_controllers_fixit_davide/four_wheel_steering_controller;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake ];

  buildInputs = [
    control-msgs
    controller-interface
    hardware-interface
    pluginlib
    rclcpp
    rclcpp-lifecycle
    std-srvs
    (steering-controllers-library.overrideAttrs {
      src = builtins.path rec {
        name = "steering_controllers_library-source";
        path = ../../../../controllers/ros2_controllers_fixit_davide/steering_controllers_library;
        filter = lib.novaSourceFilter [ ] path;
      };
    })
    generate-parameter-library
  ];
}
