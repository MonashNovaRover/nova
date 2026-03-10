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
    
    cp -r build/build_Data $out/share/
    cp build/build.x86_64 $out/share/
    cp build/libdecor-0.so.0 $out/share/
    cp build/libdecor-cairo.so $out/share/
    cp build/UnityPlayer.so $out/share/

    makeWrapper ${steam-run-free}/bin/steam-run $out/bin/nova-unity-sim \
      --add-flags $out/share/build.x86_64
  '';
}
