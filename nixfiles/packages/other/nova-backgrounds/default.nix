{ lib
, stdenvNoCC
, imagemagick
, nova-icons
}:

stdenvNoCC.mkDerivation (finalAttrs: {
  pname = "nova-backgrounds";
  version = "2023";

  nativeBuildInputs = [ imagemagick ];

  buildCommand = ''
    mkdir -p "$out/share/backgrounds/nova"
    convert \
      -resize 820x820 \
      -gravity center \
      -background '#434343' \
      -extent 3840x2160 \
      ${nova-icons}/share/icons/hicolor/1024x1024/apps/nova-logo-white-and-orange.png \
      "$out/share/backgrounds/nova/logo-dark.png"
  '';
})
