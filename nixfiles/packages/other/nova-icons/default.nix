{ lib
, stdenvNoCC
, runCommand
, imagemagick
}:

stdenvNoCC.mkDerivation (finalAttrs: {
  pname = "nova-icons";
  version = "2023";

  nativeBuildInputs = [ imagemagick ];

  buildCommand = ''
    for icon in '${./icons}'/*; do
      for size in 16 24 32 48 64 72 96 128 256 512 1024; do
        mkdir -p $out/share/icons/hicolor/''${size}x''${size}/apps
        convert \
          -resize ''${size}x''${size} \
          -gravity center \
          -background transparent \
          -extent ''${size}x''${size} \
          -format png \
          "$icon" \
          $out/share/icons/hicolor/''${size}x''${size}/apps/nova-logo-"$(basename ''${icon%.*})".png
      done
    done
  '';

  passthru.fakeNixOS = lib.makeOverridable
    ({ fakeScalable ? false }: runCommand "nova-nixos-icons" { } ''
      novaIconsOut='${finalAttrs.finalPackage}'
      for iconDir in "$novaIconsOut"/share/icons/hicolor/*/*; do
        relativeDir=''${iconDir#"$novaIconsOut"}
        mkdir -p "$out/$relativeDir"
        ln -s "$iconDir/nova-logo-white.png" "$out/$relativeDir/nix-snowflake-white.png"
        ln -s "$iconDir/nova-logo-black.png" "$out/$relativeDir/nix-snowflake.png"
      done
      ${lib.optionalString fakeScalable ''
        mkdir -p "$out/share/icons/hicolor/scalable/apps"
        ln -s "$novaIconsOut/share/icons/hicolor/1024x1024/apps/nova-logo-white.png" "$out/share/icons/hicolor/scalable/apps/nix-snowflake-white.svg"
        ln -s "$novaIconsOut/share/icons/hicolor/1024x1024/apps/nova-logo-black.png" "$out/share/icons/hicolor/scalable/apps/nix-snowflake.svg"
      ''}
    '')
    { };
})
