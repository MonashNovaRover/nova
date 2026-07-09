{ 
  stdenv,
  makeWrapper,
  steam-run-free,
  fetchurl,
  breakpointHook,
}:

# https://nixos.wiki/wiki/Packaging/Binaries
stdenv.mkDerivation rec {
  name = "nova-unity-sim";
  version = "1.0";

  src = fetchurl {
    url = "https://github.com/MonashNovaRover/unity-build/releases/download/v${version}/unity_build.tar.xz";
    hash = "sha256-IclEgjr3/yWnB6FO+N80baUq/dK11iLKyMz6H2eM9pc=";
  };

  nativeBuildInputs = [ breakpointHook ];
  buildInputs = [ steam-run-free makeWrapper ];

  sourceRoot = ".";

  installPhase = ''
    mkdir -p $out/bin
    mkdir -p $out/share
    
    cp -r unity_build/build_Data $out/share/
    cp unity_build/build.x86_64 $out/share/
    cp unity_build/libdecor-0.so.0 $out/share/
    cp unity_build/libdecor-cairo.so $out/share/
    cp unity_build/UnityPlayer.so $out/share/

    makeWrapper ${steam-run-free}/bin/steam-run $out/bin/${name} \
      --add-flags $out/share/build.x86_64
  '';

  platforms = [ "x86_64-linux" ];
}
