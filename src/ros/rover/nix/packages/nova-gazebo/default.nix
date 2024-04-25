{ lib
, buildRosPackage
, ament-cmake
, launch
, launch-ros
}:

buildRosPackage rec {
  name = "nova-gazebo";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-gazebo-source";
    path = ../../../nova_gazebo;
    filter = lib.novaSourceFilter [ "!worlds/**" ] path;
  };

  terrain = builtins.path {
    name = "nova-terrain";
    path = src + "/nova_terrain";
  };

  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ launch launch-ros ];

  postPatch = ''
    substituteInPlace  worlds/urc_er.model \
      --replace 'STREQUAL "nova_terrain_directory"' 'STREQUAL "${terrain}"'
  '';
}
