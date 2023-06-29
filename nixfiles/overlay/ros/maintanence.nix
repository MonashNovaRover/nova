self: super:

{
  rosPackages = super.rosPackages // {
    foxy = super.rosPackages.foxy.overrideScope (rosSelf: rosSuper: {
      # Use ros2doctor from Humble: https://github.com/lopsided98/nix-ros-overlay/issues/75#issuecomment-1567281292
      ros2doctor = self.rosPackages.foxy.callPackage self.rosPackages.humble.ros2doctor.override { };

      # Backported compilation error fixes.
      pendulum-control = super.rosPackages.foxy.pendulum-control.overrideAttrs ({ patches ? [ ], ... }: {
        patches = patches ++ [
          (self.fetchpatch {
            url = "https://github.com/ros2/demos/commit/754612348e408675f526174c5f03786e08ad8a70.patch";
            stripLen = 1;
            hash = "sha256-B+UW1OL0SOs7mOEOtpu5CSo8zSk5ifJdwC/deY/7zTg=";
          })
        ];
      });
    });
  };
}
