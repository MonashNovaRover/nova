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
  nova-cli = callPackage ../../src/other/nova_cli { };
}
