{ buildNpmPackage
, fetchFromGitHub
, libsForQt5
, ffmpeg
}:

buildNpmPackage {
  pname = "reolink-ctl";
  version = "0.0.1";

  src = "./src";
  #src = fetchFromGitHub {
  #  owner = "maptiler";
  #  repo = "tileserver-gl";
  #  rev = "v4.11.1";
  #  sha256 = "IZZq2trDn3HBUI4SU1rplIb1nv5mB7O4bxaAmX5M/W0=";
  #};

  npmDepsHash = "sha256-OSBBqklF/ozAjsV4Wk3HS44GfnMGRwh6GifglrUcEHM=";

  #npmInstallFlags = [ "--build-from-source=@maplibre/maplibre-gl-native" ];

  #buildInputs = [
    #libsForQt5.maplibre-gl-native
  #];
}
