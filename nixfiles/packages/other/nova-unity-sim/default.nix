{ 
  autoPatchelfHook, 
  dbus,
  fetchurl, 
  lib, 
  libGL, 
  libudev0-shim, 
  libx11, 
  libxi, 
  libxrandr, 
  makeWrapper, 
  minizip, 
  pango, 
  stdenv, 
  vulkan-loader, 
  wayland, 
  zlib, 
}:

# Summary:
# Unity compiles games into ELF executables for Linux x86, which can have static and/or dynamic dependencies.
# Static dependencies are compiled into the executable binaries, so their binaries are always available.
# Dynamic dependencies are compiled in by reference, so that reference needs to be checked and followed at runtime to find that dependency's binaries.
# Some dynamic dependencies can be linked when compiled, meaning that they are tracked by ELF and can be modified by autoPatchelfHook to link to the relevant dependency in the nix store.
# Other dynamic dependencies will only reveal themselves during runtime and check LD_LIBRARY_PATH; they do not appear in ELF and cannot be patched by autoPatchelfHook.
# These can be satisfied by wrapping the executable in an environment where LD_LIBRARY_PATH links to the relevant dependencies in the nix store. 
# Sources:
# https://github.com/NixOS/nixpkgs/blob/master/doc/hooks/autopatchelf.section.md
# https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
# https://github.com/NixOS/nixpkgs/blob/master/doc/stdenv/stdenv.chapter.md#makewrapper-executable-wrapperfile-args-fun-makewrapper
stdenv.mkDerivation (finalAttrs: {
  name = "nova-unity-sim";
  version = "1.0.0";
  build = "build";

  src = fetchurl {
    url = "https://github.com/MonashNovaRover/unity-sim/releases/download/${finalAttrs.version}/${finalAttrs.build}.tar.xz";
    hash = "sha256-Kv7CSyNJQKMuEATjBTDXIpn/u9sjAvPVJOlm7/SETN8=";
  };

  sourceRoot = ".";

  nativeBuildInputs = [
    autoPatchelfHook
    makeWrapper
  ];

  buildInputs = [
    dbus
    minizip
    pango
    wayland
  ];

  runtimeDependencies = [
    libGL
    libudev0-shim
    libx11
    libxi
    libxrandr
    vulkan-loader
    zlib
  ];

  installPhase = ''
    runHook preInstall

    mkdir -p $out/share
    cp -r ${finalAttrs.build}/* $out/share/
    mkdir -p $out/bin

    makeWrapper $out/share/${finalAttrs.build}.x86_64 $out/bin/${finalAttrs.name} \
      --prefix LD_LIBRARY_PATH : ${lib.makeLibraryPath finalAttrs.runtimeDependencies}

    runHook postInstall
  '';

  platforms = [ "x86_64-linux" ];
})
