{ lib
, buildRosPackage
, ament-cmake
, rosidl-default-generators
, launch
, launch-ros
, joint-state-publisher
, foxglove-bridge
, foxglove-msgs
, can-utils
}:

buildRosPackage {
  name = "nova-bringup";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-bringup-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  nativeBuildInputs = [ ament-cmake rosidl-default-generators ];
  propagatedBuildInputs = [ launch launch-ros joint-state-publisher  foxglove-bridge foxglove-msgs];

  postPatch = ''
    sed -i launch/can.launch.py \
      -e 's@"candump"@"${can-utils}/bin/candump"@g'
  '';
}
