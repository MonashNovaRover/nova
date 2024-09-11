{ lib
, stdenvNoCC
, librealsense2
, runtimeShell
}:

stdenvNoCC.mkDerivation {
  pname = "realsense-udev";
  version = lib.getVersion librealsense2;

  inherit (librealsense2) src;

  installPhase = ''
    mkdir -p "$out/lib/udev/rules.d"
    cp config/*.rules "$out/lib/udev/rules.d"

    mkdir -p "$out/bin"
    cp config/usb-R200-in config/usb-R200-in_udev "$out/bin"
    chmod +x "$out/bin"/*
  '';

  preFixup = ''
    substituteInPlace "$out/lib/udev/rules.d/99-realsense-libusb.rules" \
      --replace /bin/sh '${runtimeShell}' \
      --replace /usr/local/bin "$out/bin"
  '';
}
