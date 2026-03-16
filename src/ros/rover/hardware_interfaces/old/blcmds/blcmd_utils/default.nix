{ lib
, writeShellApplication
, buildRosPackage
, rclpy
, pythonPackages
, nova-blcmd-interfaces
}:

buildRosPackage {
  name = "blcmd-utils";
  buildType = "ament_python";

  src = builtins.path rec {
    name = "blcmd-utils-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [
    rclpy
    nova-blcmd-interfaces
    pythonPackages.jcan
  ];

  postInstall = ''
    mkdir -p "$out/bin"
    ln -s ${writeShellApplication {
      name = "blcmd_disable";
      text = builtins.readFile ./macros/blcmd_disable.sh;
    }}/bin/* "$out/bin"
  '';
}
