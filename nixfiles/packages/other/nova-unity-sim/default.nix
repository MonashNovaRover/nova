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
  version = "2.0";
  build = "build";

  src = fetchurl {
    url = "https://github.com/MonashNovaRover/unity-build/releases/download/v${version}/${build}.tar.xz";
    hash = "sha256-Kv7CSyNJQKMuEATjBTDXIpn/u9sjAvPVJOlm7/SETN8=";
  };

  nativeBuildInputs = [ breakpointHook ];
  buildInputs = [ steam-run-free makeWrapper ];

  sourceRoot = ".";

  installPhase = ''
    mkdir -p $out/bin
    mkdir -p $out/share
    
    cp -r ${build}/build_Data $out/share/
    cp ${build}/build.x86_64 $out/share/
    cp ${build}/libdecor-0.so.0 $out/share/
    cp ${build}/libdecor-cairo.so $out/share/
    cp ${build}/UnityPlayer.so $out/share/

    makeWrapper ${steam-run-free}/bin/steam-run $out/bin/${name} \
      --add-flags $out/share/build.x86_64
  '';

  platforms = [ "x86_64-linux" ];
}
