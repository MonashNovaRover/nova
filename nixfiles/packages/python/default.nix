{ callPackage }:

{
  jcan = callPackage ./jcan { };
  mkdocs-safe-text-plugin = callPackage ./mkdocs-safe-text-plugin { };
  ultralytics = callPackage ./ultralytics { };
  super-gradients = callPackage ./super-gradients { };
  lap = callPackage ./lap { };
  linuxpy = callPackage ./linuxpy { };
  pynmeagps = callPackage ./pynmeagps { };
  pyrtcm = callPackage ./pyrtcm { };
  pyubx2 = callPackage ./pyubx2 { };
  minimalmodbus = callPackage ./minimalmodbus { };
}
