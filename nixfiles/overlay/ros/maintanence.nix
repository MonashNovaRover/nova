self: super:

{
  rosPackages = (super.rosPackages.appendDistroOverlay
    # Overlay for all ROS distros.
    (rosSelf: rosSuper: {
      # Newer versions of realsense-ros cannot generate point clouds on Jetsons.
      # https://github.com/IntelRealSense/realsense-ros/issues/2575#issuecomment-1346319645
      realsense2-camera = rosSuper.realsense2-camera.overrideAttrs ({ propagatedBuildInputs ? [ ], postPatch ? "", ... }: {
        version = "3.2.2-r1";
        src = self.fetchzip {
          url = "https://github.com/IntelRealSense/realsense-ros-release/archive/release/foxy/realsense2_camera/3.2.2-1.tar.gz";
          name = "3.2.2-1.tar.gz";
          hash = "sha256-/XTnkm0LJK6LAsXBzULoa6wZGN01oFvAXRDg98Sn7Bc=";
        };
        propagatedBuildInputs = propagatedBuildInputs ++ [
          rosSelf.std-srvs
        ];
        postPatch = postPatch + ''
          substituteInPlace CMakeLists.txt \
            --replace 'STREQUAL "foxy"' 'STREQUAL "${rosSelf.ros-environment.rosDistro}"'
          substituteInPlace src/base_realsense_node.cpp \
            --replace '#ifdef GALACTIC' '#if 1'
        '';
      });
      realsense2-camera-msgs = rosSuper.realsense2-camera-msgs.overrideAttrs ({ ... }: {
        version = "3.2.2-r1";
        src = self.fetchzip {
          url = "https://github.com/IntelRealSense/realsense-ros-release/archive/release/foxy/realsense2_camera_msgs/3.2.2-1.tar.gz";
          name = "3.2.2-1.tar.gz";
          hash = "sha256-hZiIvKoOLRVFdyEwpfNuSGTNmfni4NwEMID8NIkGq9s=";
        };
      });
      realsense2-description = rosSuper.realsense2-description.overrideAttrs ({ ... }: {
        version = "3.2.2-r1";
        src = self.fetchzip {
          url = "https://github.com/IntelRealSense/realsense-ros-release/archive/release/foxy/realsense2_description/3.2.2-1.tar.gz";
          name = "3.2.2-1.tar.gz";
          hash = "sha256-ba9nziritxqO4BHzxW48cFwRv/cAGeE9Udj7D6uYpMY=";
        };
      });
      librealsense2 = rosSuper.librealsense2.overrideAttrs ({ ... }: {
        version = "2.48.0-r1";
        src = self.fetchzip {
          url = "https://github.com/IntelRealSense/librealsense2-release/archive/release/foxy/librealsense2/2.48.0-1.tar.gz";
          name = "2.48.0-1.tar.gz";
          hash = "sha256-44iXQiCHMTF8ZJJ8UuVU1osSBLb6cPMk41SbMlzFPLY=";
        };
      });
    })
    # Overlays for individual ROS distros.
    (super.rosPackages // {
      foxy = super.rosPackages.foxy.overrideScope (rosSelf: rosSuper:
        {
          # Use ros2doctor from Humble: https://github.com/lopsided98/nix-ros-overlay/issues/75#issuecomment-1567281292
          ros2doctor = rosSelf.callPackage self.rosPackages.humble.ros2doctor.override { };
        });
    }));
}
