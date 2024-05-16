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

      velodyne-pointcloud = rosSuper.velodyne-pointcloud.overrideAttrs ({ nativeBuildInputs ? [ ], buildInputs ? [ ], ... }: {
        nativeBuildInputs = nativeBuildInputs ++ (with self; [ pkg-config ]);

        buildInputs = buildInputs ++ (with self; [ eigen ]);
      });

      depthai-ros = rosSuper.depthai-ros.overrideAttrs ({ propagatedBuildInputs ? [ ], ... }: {
        propagatedBuildInputs = rosSelf.lib.remove rosSelf.depthai-examples propagatedBuildInputs;
      });

      depthai-ros-driver = rosSuper.depthai-ros-driver.overrideAttrs ({ propagatedBuildInputs ? [ ], ... }: {
        propagatedBuildInputs = rosSelf.lib.remove rosSelf.depthai-examples propagatedBuildInputs;
      });

      depthai-descriptions = rosSuper.depthai-descriptions.overrideAttrs ({ ... }: {
        src = self.fetchFromGitHub {
            owner = "MonashNovaRover";
            repo = "depthai-ros";
            rev = "03995b0efd295480bb5d8442f6105492a25376f5";
            hash = "sha256-D5UNe5V75mySf0eJ88vCKBx3ip7lwHdbj8wzIQgO13k=";
        };
        version = "2.9.0-r1";
        sourceRoot = "source/depthai_descriptions";
      });

      rosbridge-library = rosSuper.rosbridge-library.override {
		    python3Packages=rosSuper.python3Packages.overrideScope (pySelf: pySuper: {
			    bson = pySelf.pymongo;
		  });
	};

    } // (
      let
        fixRtabmapDependent = pkg: pkg.overrideAttrs ({ buildInputs ? [ ], ... }: {
          buildInputs = buildInputs ++ (with self; [
            # For some reason, this is not properly included from rtabmap's
            # propagatedBuildInputs.
            (pcl.override { vtk = vtkWithQt5; })
          ]);
        });
      in
      {
        inherit (self) rtabmap;

        rtabmap-ros = rosSuper.rtabmap-ros.overrideAttrs ({ propagatedBuildInputs ? [ ], ... }: {
          propagatedBuildInputs = self.lib.remove rosSelf.rtabmap-demos propagatedBuildInputs;
        });

        rtabmap-conversions = fixRtabmapDependent rosSuper.rtabmap-conversions;
        rtabmap-odom = fixRtabmapDependent rosSuper.rtabmap-odom;
        rtabmap-rviz-plugins = fixRtabmapDependent rosSuper.rtabmap-rviz-plugins;
        rtabmap-slam = fixRtabmapDependent rosSuper.rtabmap-slam;
        rtabmap-sync = fixRtabmapDependent rosSuper.rtabmap-sync;
        rtabmap-util = fixRtabmapDependent rosSuper.rtabmap-util;
        rtabmap-viz = (fixRtabmapDependent rosSuper.rtabmap-viz).overrideAttrs ({ nativeBuildInputs ? [ ], postFixup ? "", ... }: {
          nativeBuildInputs = nativeBuildInputs ++ [ self.qt5.wrapQtAppsHook ];
          postFixup = postFixup + ''
            wrapQtApp "$out/lib/rtabmap_viz/rtabmap_viz"
          '';
        });
      }
    ) // (
      let
        # Cartographer needs Google Logging 0.5.0
        # https://github.com/cartographer-project/cartographer_ros/issues/1741#issuecomment-1288152407
        cartographer-glog = self.glog.overrideAttrs rec {
          version = "0.5.0";
          src = self.fetchFromGitHub {
            owner = "google";
            repo = "glog";
            rev = "v${version}";
            sha256 = "17014q25c99qyis6l3fwxidw6222bb269fdlr74gn7pzmzg4lvg3";
          };
          patches = [
            # Fix duplicate-concatenated nix store path in cmake file, see:
            # https://github.com/NixOS/nixpkgs/pull/144561#issuecomment-960296043
            (self.fetchpatch {
              name = "glog-cmake-Fix-incorrect-relative-path-concatenation.patch";
              url = "https://github.com/google/glog/pull/733/commits/57c636c02784f909e4b5d3c2f0ecbdbb47097266.patch";
              sha256 = "1py93gkzmcyi2ypcwyj3nri210z8fmlaif51yflzmrrv507zd7bi";
            })
          ];
        };
      in
      {
        cartographer = (rosSuper.cartographer.override {
          glog = cartographer-glog;
          ceres-solver = self.ceres-solver.override {
            glog = cartographer-glog;
          };
        }).overrideAttrs ({ patches ? [ ], nativeBuildInputs ? [ ], NIX_CFLAGS_COMPILE ? "", ... }: {
          patches = patches ++ [
            # Update code to work with recent abseil changes
            # https://github.com/cartographer-project/cartographer/pull/1919
            (self.fetchpatch {
              url = "https://github.com/kkufieta/cartographer/commit/da4d12c905ccb4b6115e052f257ee92f238771cb.patch";
              hash = "sha256-SULgbAnCM9w4CARUUOYi0F5LvSwxHWzLfOMStcSj1nE=";
            })
          ];

          nativeBuildInputs = nativeBuildInputs ++ (with self; [ pkg-config ]);

          # Ideally, these flags would be given to the build system. Using the
          # Nix wrapper directly is a bit of a hack.
          #
          # Unfortunately, Cartographer does not seem to respect any of the
          # conventional ways (cmakeFlags, CFLAGS) of doing so.
          NIX_CFLAGS_COMPILE = "${NIX_CFLAGS_COMPILE} -Wno-error=maybe-uninitialized";
        });

        cartographer-ros = (rosSuper.cartographer-ros.override {
          glog = cartographer-glog;
        }).overrideAttrs ({ patches ? [ ], ... }: {
          patches = patches ++ [
            # Fix cmake to prevent multiple definitions
            # https://github.com/ros2/cartographer_ros/pull/63
            (self.fetchpatch {
              url = "https://github.com/ros2/cartographer_ros/commit/098e6bf8556070b9228b9fa7d5766238bc141a90.patch";
              stripLen = 1;
              hash = "sha256-4hdpQ8LnjT6zuCq/Cr5gACAPpfvCSoGv4BNjERtdpgQ=";
            })
          ];
        });
      }
    ) // (
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
      }
    ) // {
      aruco-opencv = rosSuper.aruco-opencv.overrideAttrs ({ buildInputs ? [ ], ... }: {
        buildInputs = buildInputs ++ [
          # aruco_opencv is not yet compatible with OpenCV 4.7.0+.
          # https://github.com/fictionlab/ros_aruco_opencv/issues/27
          #
          # The package does not actually declare a dependency on OpenCV in its
          # manifest, and so it is not included in any build input list. This
          # has not caused any issues as cv_bridge propagates OpenCV.
          #
          # By adding OpenCV 4.6.0 as a direct build input, it is used in place
          # of the propagated version.
          (self.opencv.overrideAttrs ({ postUnpack ? "", ... }: {
            version = "4.6.0";

            src = self.fetchFromGitHub {
              owner = "opencv";
              repo = "opencv";
              rev = "4.6.0";
              hash = "sha256-zPkMc6xEDZU5TlBH3LAzvB17XgocSPeHVMG/U6kfpxg=";
            };

            postUnpack =
              let
                contribSrc = self.fetchFromGitHub {
                  owner = "opencv";
                  repo = "opencv_contrib";
                  rev = "4.6.0";
                  sha256 = "sha256-hjRqT7V4Sz7t4IEy89F5M+b0x2ObBbqF8GWLKhWFXtE=";
                };
              in
              postUnpack + ''
                rm -r "$NIX_BUILD_TOP/source/opencv_contrib"
                cp --no-preserve=mode -r "${contribSrc}/modules" "$NIX_BUILD_TOP/source/opencv_contrib"
              '';
          }))
        ];
      });
    } // (
      let
        replaceUbloxSrc = pkg: pkg.overrideAttrs ({ src, version, ... }: {
          src = self.fetchFromGitHub {
            owner = "leighleighleigh";
            repo = "ublox_dgnss";
            rev = "a64e313ddbb01234c91b757c76280a5780bfd0e3";
            hash = "sha256-/R/RDaKDmMjAy1oTERqi0FtV/Zs32oFcB8ZYe1EdZmE=";
          };

          version = "0.2.3";
        });
      in
      {
        ublox-dgnss = (replaceUbloxSrc rosSuper.ublox-dgnss).overrideAttrs ({ ... }: {
          sourceRoot = "source/ublox_dgnss";
        });

        ublox-dgnss-node = (replaceUbloxSrc rosSuper.ublox-dgnss-node).overrideAttrs ({ propagatedBuildInputs ? [ ], ... }: {
          sourceRoot = "source/ublox_dgnss_node";

          propagatedBuildInputs = propagatedBuildInputs ++ (with rosSuper; [
            sensor-msgs
            geometry-msgs
          ]);
        });

        ublox-ubx-interfaces = (replaceUbloxSrc rosSuper.ublox-ubx-interfaces).overrideAttrs ({ ... }: {
          sourceRoot = "source/ublox_ubx_interfaces";
        });

        ublox-ubx-msgs = (replaceUbloxSrc rosSuper.ublox-ubx-msgs).overrideAttrs ({ ... }: {
          sourceRoot = "source/ublox_ubx_msgs";
        });
      }
    )
    )

    # Overlays for individual ROS distros.
    (super.rosPackages // {
      foxy = super.rosPackages.foxy.overrideScope (rosSelf: rosSuper:
        {
          # Use ros2doctor from Humble: https://github.com/lopsided98/nix-ros-overlay/issues/75#issuecomment-1567281292
          ros2doctor = rosSelf.callPackage self.rosPackages.humble.ros2doctor.override { };
        });
    }));

  # Overlays for non-ROS packages
}
