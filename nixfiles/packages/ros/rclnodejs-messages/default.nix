{ runCommand
, symlinkJoin
, writeTextDir
, rclnodejs-message-generator
, buildEnv
, ros-core ? throw "rosEnv has not been specified, but ros-core is not available!"
, rosEnv ? buildEnv { paths = [ ros-core ]; }
}:

(runCommand "rclnodejs-${rosEnv.name}-messages" {
  nativeBuildInputs = [
    rclnodejs-message-generator.nodejs
    rclnodejs-message-generator
    rosEnv
  ];
}) ''
  # rclnodejs attempts to modify its own library files to add message
  # definitions. Most package managers do not like this technique, but this was
  # evidently not a concern that came across the maintainers' collective
  # braincell.
  # The complete package must be copied to a writable location before use.
  mkdir -p node_modules
  cp -r ${rclnodejs-message-generator}/lib/node_modules .
  chmod -R +w node_modules
  npm run --prefix node_modules/rclnodejs generate-messages
  
  mkdir -p "$out/share/rclnodejs/messages"
  cp -r node_modules/rclnodejs/generated "$out/share/rclnodejs/messages"
  cp -r node_modules/rclnodejs/types/interfaces.d.ts "$out/share/rclnodejs/messages"
''
