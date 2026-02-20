{
  stdenv,
  lib,
  unzip,
  fetchzip,
  util-linux,
  libusb1,
  #evdi, Can't get this as it is an os specific package
  makeBinaryWrapper,
}:

let
  bins =
    if stdenv.hostPlatform.system == "x86_64-linux" then
      "x64-ubuntu-1604"
    else
      throw "Unsupported architecture";
  libPath = lib.makeLibraryPath [
    stdenv.cc.cc
    util-linux
    libusb1
    #evdi
  ];
in
stdenv.mkDerivation (finalAttrs: {
  pname = "SMIUSBDisplay";
  version = "2.24.7";

  src = fetchzip {
    url = "https://www.siliconmotion.com/downloads/SMI-USB-Display-for-Linux-v2.24.7.0.zip";
    hash = "";
  };

  nativeBuildInputs = [
    makeBinaryWrapper
    unzip
  ];

  unpackPhase = ''
    runHook preUnpack
    ls
    unzip $src
    ls
    chmod +x displaylink-driver-${finalAttrs.version}.run
    ./displaylink-driver-${finalAttrs.version}.run --target . --noexec --nodiskspace
    runHook postUnpack
  '';

  installPhase = ''
    runHook preInstall

    install -Dt $out/lib/displaylink *.spkg
    install -Dm755 ${bins}/DisplayLinkManager $out/bin/DisplayLinkManager
    mkdir -p $out/lib/udev/rules.d $out/share
    patchelf \
      --set-interpreter $(cat ${stdenv.cc}/nix-support/dynamic-linker) \
      --set-rpath ${libPath} \
      $out/bin/DisplayLinkManager
    wrapProgram $out/bin/DisplayLinkManager \
      --chdir "$out/lib/displaylink"

    # We introduce a dependency on the source file so that it need not be redownloaded everytime
    echo $src >> "$out/share/workspace_dependencies.pin"

    runHook postInstall
  '';

  dontStrip = true;
  dontPatchELF = true;

  meta = {
    mainProgram = "SMIUSBDisplayManager";
    platforms = [
      "x86_64-linux"
    ];
    sourceProvenance = with lib.sourceTypes; [ binaryNativeCode ];
  };
})
