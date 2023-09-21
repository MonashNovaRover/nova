{ lib
, stdenvNoCC
, librealsense2
,
}:

stdenvNoCC.mkDerivation {
  pname = "librealsense2-patches";
  inherit (librealsense2.original) version src;

  installPhase = ''
    mkdir "$out"
    cp -r scripts/Tegra/LRS_Patches/*.patch "$out"
  '';
}
