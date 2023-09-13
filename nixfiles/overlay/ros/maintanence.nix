self: super:

{
  rosPackages = (super.rosPackages.appendDistroOverlay
    # Overlay for all ROS distros.
    (rosSelf: rosSuper: { })
    # Overlays for individual ROS distros.
    (super.rosPackages // {
      foxy = super.rosPackages.foxy.overrideScope (rosSelf: rosSuper:
        let
          # nix-ros-overlay has existing Python overriding architecture that we need to co-operate with - hence the weird structure.
          # https://github.com/lopsided98/nix-ros-overlay/blob/4c0baef357f0d0c84502202d75a69c496feb433e/distros/distro-overlay.nix#L4
          pythonOverridesFor = prevPython: prevPython // {
            pkgs = prevPython.pkgs.overrideScope (pyself: pysuper: self.lib.optionalAttrs pysuper.isPy3k {
              # https://github.com/ros/rosdistro/pull/38361
              # As there is no rosdep key for OpenCV 3 specifically, all uses of OpenCV intend to reference OpenCV 4.
              # The PR fixing this was merged after Foxy was dropped, and needs to be replicated here.
              opencv3 = pyself.opencv4;
            });
          };
        in
        {
          python = pythonOverridesFor rosSuper.python;
          python3 = pythonOverridesFor rosSuper.python3;

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
}
