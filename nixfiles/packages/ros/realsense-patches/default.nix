{ lib
, stdenvNoCC
, librealsense2
,
}:

stdenvNoCC.mkDerivation {
  pname = "librealsense2-patches";
  version = lib.getVersion librealsense2;

  inherit (librealsense2) src;

  installPhase = ''
    mkdir "$out"
    cp -r scripts/Tegra/LRS_Patches/*.patch "$out"
  '';
}
