{ 
  fetchurl,
  lib, 
  libGL, 
  libudev0-shim, 
  libx11,
  libxi, 
  libxrandr, 
  makeWrapper, 
  stdenv,
  vulkan-loader, 
  zlib, 
}:

# https://discourse.nixos.org/t/makewrapper-vs-buildinputs/21474
# Used Claude
# The unity build requires several of the dependencies listed below to be
# available at runtime, which is achieved by linking them to the build executable.
# The dependencies were determined by muntzing.
stdenv.mkDerivation rec {
  name = "nova-unity-sim";
  version = "1.0.0";
  build = "build";

  src = fetchurl {
    url = "https://github.com/MonashNovaRover/unity-sim/releases/download/${version}/${build}.tar.xz";
    hash = "sha256-Kv7CSyNJQKMuEATjBTDXIpn/u9sjAvPVJOlm7/SETN8=";
  };

  sourceRoot = ".";

  nativeBuildInputs = [
    makeWrapper
  ];

  installPhase = ''
    runHook preInstall

    mkdir -p $out/bin $out/share
    cp -r ${build}/* $out/share/

    makeWrapper $out/share/build.x86_64 $out/bin/${name} \
      --prefix LD_LIBRARY_PATH : ${lib.makeLibraryPath [ 
        libGL
        libudev0-shim
        libx11
        libxi
        libxrandr
        vulkan-loader
        zlib
      ]}

    runHook postInstall
  '';

  platforms = [ "x86_64-linux" ];
}
