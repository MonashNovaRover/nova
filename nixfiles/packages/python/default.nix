{ callPackage }:

{
  jcan = callPackage ./jcan { };
  mkdocs-safe-text-plugin = callPackage ./mkdocs-safe-text-plugin { };
  super-gradients = callPackage ./super-gradients { };
  lap = callPackage ./lap { };
  linuxpy = callPackage ./linuxpy { };
  pynmeagps = callPackage ./pynmeagps { };
  pyrtcm = callPackage ./pyrtcm { };
  pyunigps = callPackage ./pyunigps { };
  minimalmodbus = callPackage ./minimalmodbus { };
  geomaglib = callPackage ./geomaglib { };
  wmm-calculator = callPackage ./wmm-calculator { };
  pypcd4 = callPackage ./pypcd4 { };
  ros2-unbag = callPackage ./ros2-unbag { };
  pyubx2 = callPackage ./pyubx2 { };
}
