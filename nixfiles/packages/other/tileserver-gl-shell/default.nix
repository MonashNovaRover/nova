{ buildFHSEnv }:

buildFHSEnv {
  name = "tileserver-gl-fhs";
  targetPkgs = pkgs: with pkgs; [
    nodejs
    libuuid
    libglvnd
    curl
    libjpeg8
    xorg.libX11
    libwebp
    icu70
  ];
}
