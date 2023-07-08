self: super:

{
  rosPackages = (super.rosPackages.appendDistroOverlay
    # Overlay for all ROS distros.
    (rosSelf: rosSuper: {
      # Use a working RMW implementation by default.
      # https://github.com/lopsided98/nix-ros-overlay/issues/45
      buildEnv = { paths, postBuild ? "", ... }@args: rosSuper.buildEnv {
        paths = paths ++ [
          rosSelf.rmw-fastrtps-dynamic-cpp
        ];

        postBuild = ''
          ${postBuild}
          rosWrapperArgs+=(--set-default RMW_IMPLEMENTATION rmw_fastrtps_dynamic_cpp)
        '';
      };

      # Use X11 by default in RViz2.
      # https://github.com/ros-visualization/rviz/issues/1442
      rviz2 = rosSuper.rviz2.overrideAttrs ({ qtWrapperArgs ? [ ], ... }: {
        qtWrapperArgs = qtWrapperArgs ++ [ "--set-default QT_QPA_PLATFORM xcb" ];
      });
    })
    # Overlays for individual ROS distros.
    (super.rosPackages // {
      foxy = super.rosPackages.foxy.overrideScope (rosSelf: rosSuper: {
        # Use ros2doctor from Humble: https://github.com/lopsided98/nix-ros-overlay/issues/75#issuecomment-1567281292
        ros2doctor = rosSelf.callPackage self.rosPackages.humble.ros2doctor.override { };

        # Add argcomplete as a propagated ros2cli dependency.
        # https://github.com/ros2/ros2cli/pull/564
        # https://github.com/ros2/ros2cli/blob/26715cbb0948258d6f04b94c909d035c5130456a/ros2cli/ros2cli/cli.py#L45
        ros2cli = rosSuper.ros2cli.overrideAttrs ({ propagatedBuildInputs ? [ ], ... }: {
          propagatedBuildInputs = propagatedBuildInputs ++ [ rosSelf.pythonPackages.argcomplete ];
        });

        # Backported compilation error fixes.
        pendulum-control = rosSuper.pendulum-control.overrideAttrs ({ patches ? [ ], ... }: {
          patches = patches ++ [
            (self.fetchpatch {
              url = "https://github.com/ros2/demos/commit/754612348e408675f526174c5f03786e08ad8a70.patch";
              stripLen = 1;
              hash = "sha256-B+UW1OL0SOs7mOEOtpu5CSo8zSk5ifJdwC/deY/7zTg=";
            })
          ];
        });
      });
    }));
}
