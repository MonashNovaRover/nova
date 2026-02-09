{ pkgs, callPackage }:

{
  github-gitignore = callPackage ./github-gitignore { };
  libgphoto2-theta = callPackage ./libgphoto2-theta { };
  groot2 = callPackage ({ libsForQt5 }: libsForQt5.callPackage ./groot2 { }) { };
  gstthetauvc = callPackage ./gstthetauvc { };
  jcan = callPackage ./jcan { };
  libuvc-theta = callPackage ./libuvc-theta { };
  novafox = callPackage ./novafox { } { };
  nova-backgrounds = callPackage ./nova-backgrounds { };
  nova-icons = callPackage ./nova-icons { };
  ros-typescript-generator = callPackage ./ros-typescript-generator { };
  tileserver-gl = callPackage ./tileserver-gl { };
  tileserver-gl-shell = callPackage ./tileserver-gl-shell { };
  foxglove = callPackage ./foxglove { };
  reolink = callPackage ./reolink { };
  ptpd = callPackage ./ptpd { };
} // import ./nova-launch { inherit pkgs; }
