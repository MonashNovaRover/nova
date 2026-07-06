{ lib
, buildPythonPackage
, argcomplete
}:

buildPythonPackage {
  pname = "nova-cli";
  version = "1.0.0";

  src = builtins.path rec {
    name = "nova-cli-source";
    path = ./.;
    filter = lib.novaSourceFilter [ ] path;
  };

  propagatedBuildInputs = [ argcomplete ];

  # Generate and install bash completion
  postInstall = ''
    # Generate completion script
    mkdir -p $out/share/bash-completion/completions

    # Use Python to generate the completion script via argcomplete
    ${argcomplete}/bin/register-python-argcomplete nova > $out/share/bash-completion/completions/nova
  '';

  meta = with lib; {
    description = "Nova ROS2 CLI wrapper tool";
    homepage = "https://github.com/MonashNovaRover/nova";
    license = licenses.asl20;
    maintainers = [ ];
  };
}
