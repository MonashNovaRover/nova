{ runCommand
, writeText
, ros-typescript-generator
, buildEnv
, ros-environment
, ros-core ? throw "rosEnv has not been specified, but ros-core is not available!"
, rosEnv ? buildEnv { paths = [ ros-core ]; }
, typePrefix ? "IRosType"
}:

(runCommand "ros-${rosEnv.name}-typescript-definitions" {
  nativeBuildInputs = [
    ros-typescript-generator
  ];
}) ''
  mkdir -p generated
  ln -s ${writeText "ros-ts-generator-config.json" (builtins.toJSON {
    output = "./generated/messages.ts";
    inherit (ros-environment) rosVersion;
    inherit typePrefix;
    input = map (name: {
      namespace = name;
      path = rosEnv + /share/${name};
    }) (builtins.attrNames (builtins.readDir (rosEnv + /share)));
  })} ros-ts-generator-config.json
  ros-typescript-generator
  mkdir -p "$out/share/ros-typescript-definitions"
  cp -r generated/* "$out/share/ros-typescript-definitions"
''
