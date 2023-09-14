self: super:

{
  rosPackages = (super.rosPackages.appendDistroOverlay
    # Overlay for all ROS distros.
    (rosSelf: rosSuper: { })
    # Overlays for individual ROS distros.
    (super.rosPackages // {
      foxy = super.rosPackages.foxy.overrideScope (rosSelf: rosSuper:
        {
          # Use ros2doctor from Humble: https://github.com/lopsided98/nix-ros-overlay/issues/75#issuecomment-1567281292
          ros2doctor = rosSelf.callPackage self.rosPackages.humble.ros2doctor.override { };
        });
    }));
}
