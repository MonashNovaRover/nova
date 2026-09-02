{ 
  lib, 
  stdenv,
  makeWrapper, 
  fetchurl,
  autoPatchelfHook, 
  autoAddDriverRunpath, 
  zlib, 
  alsa-lib, 
  pulseaudio, 
  libGL, 
  glib, 
  gtk3, 
  cairo, 
  pango, 
  gdk-pixbuf, 
  fontconfig, 
  freetype, 
  libxkbcommon, 
  minizip, 
  xorg, 
  libpulseaudio, 
  libudev0-shim, 
  libxcursor, 
  libxi, 
  libxinerama, 
  libxrandr, 
  libxscrnsaver, 
  libxxf86vm, 
  vulkan-loader, 
  wayland, 
}:

# https://nixos.wiki/wiki/Packaging/Binaries
stdenv.mkDerivation rec {
  name = "nova-unity-sim";
  version = "1.0.0";
  build = "build";

  src = fetchurl {
    url = "https://github.com/MonashNovaRover/unity-sim/releases/download/${version}/${build}.tar.xz";
    hash = "sha256-Kv7CSyNJQKMuEATjBTDXIpn/u9sjAvPVJOlm7/SETN8=";
  };

  buildInputs = [
    stdenv.cc.cc.lib
    zlib
    alsa-lib
    alsa-lib
    libGL
    libpulseaudio
    libudev0-shim
    libxcursor
    libxi
    libxinerama
    libxrandr
    libxscrnsaver
    libxxf86vm
    vulkan-loader
    zlib
    pulseaudio
    libGL
    glib
    gtk3
    cairo
    pango
    gdk-pixbuf
    fontconfig
    freetype
    libxkbcommon
    minizip
    wayland
  ] ++ (with xorg; [
    libX11
    libXext
    libXxf86vm
  ]);

  sourceRoot = ".";

  nativeBuildInputs = [
    autoPatchelfHook
    autoAddDriverRunpath
    makeWrapper
  ];

  installPhase = ''
    runHook preInstall

    mkdir -p $out/bin $out/share

    cp -r ${build}/build_Data $out/share/
    cp ${build}/build.x86_64 $out/share/
    cp ${build}/libdecor-0.so.0 $out/share/
    cp ${build}/libdecor-cairo.so $out/share/
    cp ${build}/UnityPlayer.so $out/share/
    chmod +x $out/share/build.x86_64

    makeWrapper $out/share/build.x86_64 $out/bin/${name} \
      --chdir $out/share \
      --set LIBDECOR_PLUGIN_DIR $out/share \
      --prefix LD_LIBRARY_PATH : ${lib.makeLibraryPath [
        xorg.libX11
        xorg.libXcursor
        xorg.libXrandr
        xorg.libXinerama
        xorg.libXi
        xorg.libXext
        xorg.libXxf86vm
        libxkbcommon
        wayland
        libGL
        vulkan-loader
      ]}

    runHook postInstall
  '';

  platforms = [ "x86_64-linux" ];
}
