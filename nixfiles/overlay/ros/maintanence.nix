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
    })
    # Overlays for individual ROS distros.
    (super.rosPackages // {
      humble = super.rosPackages.humble.overrideScope (rosSelf: rosSuper: {
        # rviz-common is missing a NixOS dependency in rosdistro.
        # https://github.com/ros/rosdistro/pull/38136
        rviz-common = (rosSelf.callPackage "${self.nix-ros-overlay}/distros/humble/rviz-common" { }).overrideAttrs ({ buildInputs ? [ ], propagatedBuildInputs ? [ ], ... }: {
          version = assert rosSelf.rviz2.version == "11.2.6-r1"; rosSelf.rviz2.version;

          src = self.fetchurl {
            url = "https://github.com/ros2-gbp/rviz-release/archive/release/humble/rviz_common/11.2.6-1.tar.gz";
            name = "11.2.6-1.tar.gz";
            sha256 = "sha256-cnGdWmfWs6VWNIhlzsOph1RtGV/9kNiL0VSTs90YlNQ=";
          };

          # The set build inputs here are a little strange.
          # This is to match the expected output from Super Flore once the
          # relevant PR lands, to avoid rebuilds later.
          buildInputs = buildInputs ++ (with self; [ qt5.qtsvg.dev ]);
          propagatedBuildInputs = propagatedBuildInputs ++ (with self; [ qt5.qtsvg ]);
        });
      });

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

        rviz-ogre-vendor = rosSuper.rviz-ogre-vendor.overrideAttrs ({ patches ? [ ], ... }: {
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
        });
      });
    }));
}
