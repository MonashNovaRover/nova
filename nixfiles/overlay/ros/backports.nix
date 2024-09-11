self: super:

{
  rosPackages =
    (super.rosPackages.appendDistroOverlay
      # Overlay for all ROS distros.
      (rosSelf: rosSuper: {
        rosbridge-library = rosSuper.rosbridge-library.overrideAttrs {
          version = "1.3.2-unstable-2024-02-12";
          src = self.fetchFromGitHub {
            owner = "RobotWebTools";
            repo = "rosbridge_suite";
            rev = "7d78af16d30d0ffe232abcc65d0928ce90bd61f7";
            hash = "sha256-geWbNboZRm6Sr4+aWVTVjPThi8eUYNDZ+MbrHdbWuIo=";
          };
          sourceRoot = "source/rosbridge_library";
        };

        rosbridge-msgs = rosSuper.rosbridge-msgs.overrideAttrs {
          inherit (rosSelf.rosbridge-library) version src;
          sourceRoot = "source/rosbridge_msgs";
        };

        rosbridge-server = rosSuper.rosbridge-server.overrideAttrs {
          inherit (rosSelf.rosbridge-library) version src;
          sourceRoot = "source/rosbridge_server";
        };
      }))

      # Overlays for individual ROS distros.
      (super.rosPackages // { });
}
