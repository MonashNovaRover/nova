{ callPackage }:

{
  github-gitignore = callPackage ./github-gitignore { };
  libgphoto2-theta = callPackage ./libgphoto2-theta { };
  gstthetauvc = callPackage ./gstthetauvc { };
  jcan = callPackage ./jcan { };
  libuvc-theta = callPackage ./libuvc-theta { };
  novafox = callPackage ./novafox { } { };
  nova-backgrounds = callPackage ./nova-backgrounds { };
  nova-icons = callPackage ./nova-icons { };
  ros-typescript-generator = callPackage ./ros-typescript-generator { };
}
