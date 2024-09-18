self: super:

{
  rosPackages =
    (super.rosPackages.appendDistroOverlay
      # Overlay for all ROS distros.
      (rosSelf: rosSuper: { }))

      # Overlays for individual ROS distros.
      (super.rosPackages //

        {
          jazzy = super.rosPackages.jazzy.overrideScope (rosSelf: rosSuper:
            # depthai-ros does not have a Jazzy release yet, so re-use the Iron
            # derivations with the WIP Jazzy branch.
            let
              mkDepthAIJazzy = sourceRoot: (rosSelf.callPackage (self.nix-ros-overlay + "/distros/iron/${builtins.replaceStrings [ "_" ] [ "-" ] sourceRoot}") { }).overrideAttrs ({ pname, ... }: {
                pname = builtins.replaceStrings [ "-iron" ] [ "-jazzy" ] pname;

                src = self.fetchFromGitHub {
                  owner = "luxonis";
                  repo = "depthai-ros";
                  rev = "2e81008e08ef38ccb54140f097cc7e6e27b8e572"; # https://github.com/luxonis/depthai-ros/tree/jazzy
                  hash = "sha256-VqOjki0rakeizGASphwV/YuR2ZEO5+DdxvvaoamLUTo=";
                };

                sourceRoot = "source/" + sourceRoot;
              });
            in
            {
              depthai-ros = mkDepthAIJazzy "depthai-ros";

              depthai-bridge = (mkDepthAIJazzy "depthai_bridge").overrideAttrs ({ buildInputs ? [ ], ... }: {
                buildInputs = buildInputs ++ [ rosSelf.ament-index-cpp ];
              });

              depthai-descriptions = mkDepthAIJazzy "depthai_descriptions";

              depthai-examples = mkDepthAIJazzy "depthai_examples";

              depthai-filters = mkDepthAIJazzy "depthai_filters";

              depthai-ros-driver = mkDepthAIJazzy "depthai_ros_driver";

              depthai-ros-msgs = mkDepthAIJazzy "depthai_ros_msgs";
            }
          );
        });
}
