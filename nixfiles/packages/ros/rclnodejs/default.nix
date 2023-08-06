{ lib
, hostPlatform
, runCommand
, buildNpmPackage
, fetchFromGitHub
, nodejs
, python
, colcon
, ros-environment
, rclcpp
, rcl-action
, rcl-lifecycle
, buildEnv
, ros-core ? throw "rosEnv has not been specified, but ros-core is not available!"
, rosEnv ? buildEnv { paths = [ ros-core ]; }
}:

let
  rclnodejs =
    (buildNpmPackage.override { inherit nodejs; }) rec {
      pname = "rclnodejs";
      version = "0.22.2";

      src = fetchFromGitHub {
        owner = "RobotWebTools";
        repo = pname;
        rev = version;
        hash = "sha256-7m5MU1nPguW32UpN9beVvWQh+TBGdGV772iQ+mseQvA=";
      };

      postPatch = ''
        cp ${./package-lock.json} package-lock.json
      '';

      npmDepsHash = "sha256-kaUKZE9ApA4qDuKZrjsnq8/9gVhDHoPu8EwQtdzzASw=";

      nativeBuildInputs = [
        python

        # For some reason, a CMake ROS package needs to be here for the ROS
        # environment to be detected.
        rclcpp
      ];

      buildInputs = [
        ros-environment
        rclcpp
        rcl-action
        rcl-lifecycle
      ];

      dontUseCmakeConfigure = true;

      postInstall =
        let
          bindingsDir = "\"$out\"/lib/node_modules/rclnodejs/addon-build/release/install-root";
        in
        ''
          mkdir -p ${bindingsDir}
          cp "$NIX_BUILD_TOP"/source/build/Release/*.node ${bindingsDir}
        '';

      passthru = { inherit nodejs; };
    };
in
(
  # The actual message generation is done in a separate derivation, as any paths
  # in AMENT_PREFIX_PATH will be appended to this package's compilation commands,
  # which can cause crashes if there are too many.
  # https://github.com/lopsided98/nix-ros-overlay/issues/285
  runCommand "rclnodejs-${rosEnv.name}-messages" {
    nativeBuildInputs = [
      rclnodejs.nodejs
      rosEnv
    ];
  }
) ''
  # rclnodejs attempts to modify its own library files to add message
  # definitions. Most package managers do not like this technique, but this was
  # evidently not a concern that came across the maintainers' collective
  # braincell.
  # The complete package must be copied to a writable location before use.
  mkdir -p node_modules
  cp -r '${rclnodejs}' "$out"
  chmod -R u+w "$out"
  npm run --prefix "$out/lib/node_modules/rclnodejs" generate-messages
''
