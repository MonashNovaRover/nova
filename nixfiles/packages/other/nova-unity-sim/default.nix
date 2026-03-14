{ 
  stdenv,
  makeWrapper,
  steam-run-free,
  }:

stdenv.mkDerivation {
  name = "nova-unity-sim";

  src = ./src;

  buildInputs = [ steam-run-free makeWrapper ];

  installPhase = ''
    mkdir -p $out/bin
    mkdir -p $out/share
    
    cp -r unity_build/build_Data $out/share/
    cp unity_build/build.x86_64 $out/share/
    cp unity_build/libdecor-0.so.0 $out/share/
    cp unity_build/libdecor-cairo.so $out/share/
    cp unity_build/UnityPlayer.so $out/share/

    makeWrapper ${steam-run-free}/bin/steam-run $out/bin/nova-unity-sim \
      --add-flags $out/share/build.x86_64
  '';

  platforms = [ "x86_64-linux" ];
}
