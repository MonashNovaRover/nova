{ lib
, buildRosPackage
, ament-cmake
, launch
, launch-ros
, stdenv
}:

let
  terrain = stdenv.mkDerivation {
    name = "nova-terrain";
    src = builtins.path rec {
      name = "nova_terrain";
      path = ../../../nova_gazebo/nova_terrain;
      filter = lib.novaSourceFilter [ ] path;
    };
    dontBuild = true;

    postPatch = ''
      substituteInPlace urc_er_terrain/model.sdf \
        --replace "model://urc_er_terrain/" ""

      substituteInPlace urc_auto_terrain/model.sdf \
        --replace "model://urc_er_terrain/" "../urc_er_terrain/" \
        --replace "model://urc_auto_terrain/" ""
    '';

    installPhase = ''
      mkdir -p $out
      cp -r ./ $out/
    '';
  };
in

buildRosPackage rec {
  name = "nova-gazebo";
  buildType = "ament_cmake";

  src = builtins.path rec {
    name = "nova-gazebo-source";
    path = ../../../nova_gazebo;
    filter = lib.novaSourceFilter [ "!worlds/**" ] path;
  };

  nativeBuildInputs = [ ament-cmake ];
  propagatedBuildInputs = [ launch launch-ros ];

  postPatch = ''
    substituteInPlace worlds/urc_er.model \
      --replace '/nova_terrain_directory' '${terrain}'
  '';
}
