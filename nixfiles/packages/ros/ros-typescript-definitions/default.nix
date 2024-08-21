{ runCommand
, jq
, ros-typescript-generator
, buildEnv
, ros-environment
, ros-core ? throw "rosEnv has not been specified, but ros-core is not available!"
, rosEnv ? buildEnv { paths = [ ros-core ]; }
, typePrefix ? "IRosType"
}:

(runCommand "ros-${rosEnv.name}-typescript-definitions" {
  nativeBuildInputs = [
    jq
    ros-typescript-generator
  ];
}) ''
  mkdir -p generated
  find -L '${rosEnv + /share}' -maxdepth 1 -type d -print0 | while IFS= read -r -d "" d; do echo "{\"namespace\": \""$(basename "$d")"\", \"path\": \"$d\"}"; done | jq --slurp \
    --arg output ./generated/messages.ts \
    --argjson rosVersion '${toString ros-environment.rosVersion}' \
    --arg typePrefix '${typePrefix}' \
    '{output: $output, rosVersion: $rosVersion, typePrefix: $typePrefix, input: .}' > ros-ts-generator-config.json
  ros-typescript-generator
  mkdir -p "$out/share/ros-typescript-definitions"
  cp -r generated/* "$out/share/ros-typescript-definitions"
''
