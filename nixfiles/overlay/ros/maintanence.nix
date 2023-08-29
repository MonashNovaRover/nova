self: super:

{
  rosPackages = (super.rosPackages.appendDistroOverlay
    # Overlay for all ROS distros.
    (rosSelf: rosSuper: {
      # Use X11 by default in RViz2.
      # https://github.com/ros-visualization/rviz/issues/1442
      rviz2 = rosSuper.rviz2.overrideAttrs ({ qtWrapperArgs ? [ ], ... }: {
        qtWrapperArgs = qtWrapperArgs ++ [ "--set-default QT_QPA_PLATFORM xcb" ];
      });

      pythonPackages = rosSuper.pythonPackages.overrideScope (pyself: pysuper: {
        # https://github.com/ros/rosdistro/pull/38361
        # As there is no rosdep key for OpenCV 3 specifically, all uses of OpenCV intend to reference OpenCV 4.
        opencv3 = pyself.opencv4;
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

        rviz-ogre-vendor = rosSuper.rviz-ogre-vendor.overrideAttrs ({ patches ? [ ], preFixup ? "", ... }: {
          patches = patches ++
            # Fix AArch64 builds of RViz2.
            # While this patch should not break builds on non-ARM platforms,
            # applying it invalidates the upstream binary cache. It is therefore
            # only used on platforms where it is needed.
            self.lib.optional
              self.hostPlatform.isAarch64
              (self.fetchpatch {
                url = "https://github.com/ros2/rviz/pull/828.patch";
                stripLen = 1;
                hash = "sha256-KpY9+oOsFxH+zhIxyP6UTOXTLaaUdCRzUMZnM7+uRAk=";
              });

          preFixup = ''
            # Prevent /build RPATH references
            rm -r ogre_install
          '' + preFixup;
        });
      });
    }));

  # Overlay for distro-agnostic packages.
  gazebo_11 = super.gazebo_11.overrideAttrs ({ qtWrapperArgs ? [ ], ... }: {
    qtWrapperArgs = qtWrapperArgs ++ [
      # Let the gazebo binary see neighboring binaries.
      # It attempts to run gzclient from PATH.
      "--prefix PATH : ${placeholder "out"}/bin"

      # Prevent Gazebo from attempting to use Wayland.
      # As is the case with RViz2, OGRE does not yet support it.
      "--set WAYLAND_DISPLAY dummy" # "dummy" is arbitrary - it just doesn't exist.
    ];
  });

  ignition =
    let
      fixMsgs = pkg: pkg.overrideAttrs ({ patches, ... }: {
        patches = patches ++ [
          # GzProtobuf: Do not require version 3 to support Protobuf 4.23.2 (23.2)
          (self.fetchpatch {
            url = "https://github.com/gazebosim/gz-msgs/commit/0c0926c37042ac8f5aeb49ac36101acd3e084c6b.patch";
            hash = "sha256-QnR1WtB4gbgyJKbQ4doMhfSjJBksEeQ3Us4y9KqCWeY=";
          })
        ];
      });
    in
    super.ignition // {
      msgs1 = fixMsgs super.ignition.msgs1;
      msgs5 = fixMsgs super.ignition.msgs5;
      msgs8 = fixMsgs super.ignition.msgs8;
    };
}
