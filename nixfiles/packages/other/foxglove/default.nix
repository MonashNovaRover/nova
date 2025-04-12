### Credit: Leigh Oliver
{
  dpkg,
  autoPatchelfHook,
  stdenv,
  glib,
  xorg,
  nss,
  nspr,
  cups,
  atk,
  dbus,
  dbus-glib,
  libdrm,
  gtk3,
  pango,
  cairo,
  udev,
  freetype,
  fontconfig,
  mesa,
  expat,
  libxkbcommon,
  alsa-lib,
  fetchurl,
}:

stdenv.mkDerivation rec {
  name = "foxglove-studio-v${version}";
  version = "2.19.2";

  # the actual app is extracted out of a .deb package
  src = fetchurl {
    url = "https://get.foxglove.dev/desktop/v2.19.2/foxglove-studio-2.19.2-linux-amd64.deb";
    sha256 = "sha256-PzChwhUebsnGZx9nWD/dZWu4Zog0AQ8qGNOy+nUwpm8=";
  };

  nativeBuildInputs = [
    dpkg
    autoPatchelfHook
  ];

  buildInputs =
    [
      nss
      nspr
      glib
      cups
      atk
      dbus
      dbus-glib
      libdrm
      gtk3
      pango
      cairo
      udev
      freetype
      mesa
      expat
      libxkbcommon
      alsa-lib
      fontconfig
    ]
    ++ (with xorg; [
      libSM
      libICE
      libXrender
      libXrandr
      libXfixes
      libXcursor
      libXcomposite
      libXdamage
      libxcb
      libX11
      libXext
    ]);

  # this is needed alongside with autoPatchelfHook to make foxglove happy
  runtimeDependencies = buildInputs;

  dontBuild = true;
  dontPatchELF = true;

  unpackCmd = "mkdir tmp && dpkg -x $curSrc $_";

  installPhase = ''
    cp -r . $out
    mkdir -p $out/bin
    ln -sr "$out/opt/Foxglove Studio/foxglove-studio" $out/bin/foxglove-studio
    ln -sr $out/usr/share $out/share
    # fix the Exec path in the desktop shortcut
    substituteInPlace $out/share/applications/foxglove-studio.desktop \
      --replace "/opt/Foxglove Studio/foxglove-studio" "$out/opt/Foxglove Studio/foxglove-studio"
    runHook postInstall
  '';
}
