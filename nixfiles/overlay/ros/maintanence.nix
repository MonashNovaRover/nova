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
        passthru.original = {
          inherit (rosSuper.librealsense2) version src;
        };
      });
    } // (
      let
        fixNav2Package = pkg: pkg.overrideAttrs ({ CXXFLAGS ? "", ... }: {
          # https://answers.ros.org/question/379173
          CXXFLAGS = "${CXXFLAGS} -Wno-error=maybe-uninitialized";
        });
      in
      {
        nav2-behaviors = fixNav2Package rosSuper.nav2-behaviors;
        nav2-constrained-smoother = fixNav2Package rosSuper.nav2-constrained-smoother;
        nav2-planner = fixNav2Package rosSuper.nav2-planner;
        nav2-smoother = fixNav2Package rosSuper.nav2-smoother;
        nav2-waypoint-follower = fixNav2Package rosSuper.nav2-waypoint-follower;
        dwb-critics = fixNav2Package rosSuper.dwb-critics;
        dwb-plugins = fixNav2Package rosSuper.dwb-plugins;

        ompl = rosSuper.ompl.overrideAttrs ({ patches ? [ ], ... }: {
          patches = patches ++ [
            # Use full install paths for pkg-config
            (self.fetchpatch {
              url = "https://github.com/hacker1024/ompl/commit/1ddecbad87b454ac0d8e1821030e4cf7eeff2db2.patch";
              hash = "sha256-sAQLrWHoR/DhHk8TtUEy8E8VXqrvtXl2BGS5UvElJl8=";
            })
          ];
        });

        slam-toolbox = (rosSuper.slam-toolbox.override {
          # https://github.com/ros/rosdistro/pull/38642
          tbb = self.tbb_2021_8;
        });
      }
    ))
    # Overlays for individual ROS distros.
    (super.rosPackages // {
      foxy = super.rosPackages.foxy.overrideScope (rosSelf: rosSuper:
        {
          # Use ros2doctor from Humble: https://github.com/lopsided98/nix-ros-overlay/issues/75#issuecomment-1567281292
          ros2doctor = rosSelf.callPackage self.rosPackages.humble.ros2doctor.override { };
        });
    }));

  # Overlays for non-ROS packages
  ignition = super.ignition // (
    let
      fixCommon = common: common.overrideAttrs ({ patches, ... }: {
        patches = patches ++ [
          # Fix for ffmpeg v6
          (self.fetchpatch {
            url = "https://github.com/gazebosim/gz-common/commit/d6024ce4acd3a961e3d026e5bc1bfbcb1e4b99e6.patch";
            hash = "sha256-4Iu7GQ/BsvpzBkloO3LIsMiN/STHRhbhMMAs4d1FzrY=";
          })
        ];
      });
    in
    {
      common3 = fixCommon super.ignition.common3;
      common4 = fixCommon super.ignition.common4;
    }
  );

  gazebo_11 = super.gazebo_11.overrideAttrs ({ patches ? [ ], ... }: {
    patches = patches ++ [
      # Fix build with graphviz 9
      (self.fetchpatch {
        url = "https://github.com/gazebosim/gazebo-classic/commit/87ac01bd72c7b35217ab9ebf69cba69dc7780b39.patch";
        hash = "sha256-HJyQ4yzehhbqzO+5IUVvKnRBg7rOEIZJqqRm8LzpAkc=";
      })
    ];
  });
}
