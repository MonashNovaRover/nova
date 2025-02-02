
self: super:

{
  rosPackages = (
    super.rosPackages.appendDistroOverlay
      # Overlay for all ROS distros.
      (
        rosSelf: rosSuper:
        {
          fastrtps = rosSuper.fastrtps.overrideAttrs (
            {
            ...
            }:
            {
              src = self.fetchurl {
                url = "https://github.com/ros2-gbp/fastrtps-release/archive/release/jazzy/fastrtps/2.14.1-1.tar.gz";
                name = "2.14.4-1.tar.gz";
                hash = "sha256-3E1qecQ22aoYCmOvNOWmtjqm4Q4nwn43wFsczKnoDhM=";
              };
            }
          );

          # Add ninja to cmake for faster builds
          buildRosPackage =
            {
              buildType ? "catkin",
              nativeBuildInputs ? [ ],
              ...
            }@args:
            rosSuper.buildRosPackage (
              args
              // {
                nativeBuildInputs =
                  nativeBuildInputs ++ (self.lib.optionals (buildType == "ament_cmake") [ self.ninja ]);
              }
            );

          # Newer versions of realsense-ros cannot generate point clouds on Jetsons.
          # https://github.com/IntelRealSense/realsense-ros/issues/2575#issuecomment-1346319645
          realsense2-camera = rosSuper.realsense2-camera.overrideAttrs (
            {
              propagatedBuildInputs ? [ ],
              postPatch ? "",
              ...
            }:
            {
              version = "3.2.2-r1";
              src = self.fetchzip {
                url = "https://github.com/IntelRealSense/realsense-ros-release/archive/release/foxy/realsense2_camera/3.2.2-1.tar.gz";
                name = "3.2.2-1.tar.gz";
                hash = "sha256-/XTnkm0LJK6LAsXBzULoa6wZGN01oFvAXRDg98Sn7Bc=";
              };
              propagatedBuildInputs = propagatedBuildInputs ++ [
                rosSelf.std-srvs
              ];
              postPatch =
                postPatch
                + ''
                  substituteInPlace CMakeLists.txt \
                    --replace 'STREQUAL "foxy"' 'STREQUAL "${rosSelf.ros-environment.rosDistro}"'
                  substituteInPlace src/base_realsense_node.cpp \
                    --replace '#ifdef GALACTIC' '#if 1'
                  sed -i '1i#include <cstdint>' include/constants.h
                '';
            }
          );
          realsense2-camera-msgs = rosSuper.realsense2-camera-msgs.overrideAttrs (
            { ... }:
            {
              version = "3.2.2-r1";
              src = self.fetchzip {
                url = "https://github.com/IntelRealSense/realsense-ros-release/archive/release/foxy/realsense2_camera_msgs/3.2.2-1.tar.gz";
                name = "3.2.2-1.tar.gz";
                hash = "sha256-hZiIvKoOLRVFdyEwpfNuSGTNmfni4NwEMID8NIkGq9s=";
              };
            }
          );
          realsense2-description = rosSuper.realsense2-description.overrideAttrs (
            { ... }:
            {
              version = "3.2.2-r1";
              src = self.fetchzip {
                url = "https://github.com/IntelRealSense/realsense-ros-release/archive/release/foxy/realsense2_description/3.2.2-1.tar.gz";
                name = "3.2.2-1.tar.gz";
                hash = "sha256-ba9nziritxqO4BHzxW48cFwRv/cAGeE9Udj7D6uYpMY=";
              };
            }
          );
          librealsense2 = rosSuper.librealsense2.overrideAttrs (
            {
              prePatch ? "",
              postPatch ? "",
              ...
            }:
            {
              version = "2.48.0-r1";
              src = self.fetchzip {
                url = "https://github.com/IntelRealSense/librealsense2-release/archive/release/foxy/librealsense2/2.48.0-1.tar.gz";
                name = "2.48.0-1.tar.gz";
                hash = "sha256-44iXQiCHMTF8ZJJ8UuVU1osSBLb6cPMk41SbMlzFPLY=";
              };
              prePatch =
                prePatch
                + ''
                  # This line exists in newer versions, and is removed later by nix-ros-overlay.
                  echo 'include(CMake/external_json.cmake)' >> third-party/CMakeLists.txt
                '';
              postPatch =
                postPatch
                + ''
                  sed -i '1i#include <cstdint>' common/sw-update/http-downloader.h
                '';
              passthru.original = {
                inherit (rosSuper.librealsense2) version src;
              };
            }
          );

          velodyne-pointcloud = rosSuper.velodyne-pointcloud.overrideAttrs (
            {
              nativeBuildInputs ? [ ],
              buildInputs ? [ ],
              ...
            }:
            {
              nativeBuildInputs = nativeBuildInputs ++ (with self; [ pkg-config ]);

              buildInputs = buildInputs ++ (with self; [ eigen ]);
            }
          );

          depthai-ros = rosSuper.depthai-ros.overrideAttrs (
            {
              propagatedBuildInputs ? [ ],
              ...
            }:
            {
              propagatedBuildInputs = rosSelf.lib.remove rosSelf.depthai-examples propagatedBuildInputs;
            }
          );

          depthai-ros-driver = rosSuper.depthai-ros-driver.overrideAttrs (
            {
              propagatedBuildInputs ? [ ],
              ...
            }:
            {
              propagatedBuildInputs = rosSelf.lib.remove rosSelf.depthai-examples propagatedBuildInputs;
            }
          );

          depthai-descriptions = rosSuper.depthai-descriptions.overrideAttrs (
            { ... }:
            {
              version = "2.9.0-r1";
              src = self.fetchFromGitHub {
                owner = "MonashNovaRover";
                repo = "depthai-ros";
                rev = "03995b0efd295480bb5d8442f6105492a25376f5";
                hash = "sha256-D5UNe5V75mySf0eJ88vCKBx3ip7lwHdbj8wzIQgO13k=";
              };
              sourceRoot = "source/depthai_descriptions";
            }
          );

          rosbridge-library = rosSuper.rosbridge-library.override {
            python3Packages = rosSuper.python3Packages.overrideScope (
              pySelf: pySuper: {
                bson = pySelf.pymongo;
              }
            );
          };

          grid-map-cv = rosSuper.grid-map-cv.overrideAttrs (
            {
              CXXFLAGS ? "",
              ...
            }:
            {
              CXXFLAGS = "${CXXFLAGS} -Wno-error=stringop-overflow";
            }
          );

          # osqp-vendor CMakeLists patched to not try to pull from osqp during build
          osqp-vendor =
            (rosSelf.lib.patchExternalProjectGit rosSuper.osqp-vendor {
              url = "https://github.com/osqp/osqp.git";
              originalRev = "";
              rev = "v0.6.2";
              fetchgitArgs.hash = "sha256-RYk3zuZrJXPcF27eMhdoZAio4DZ+I+nFaUEg1g/aLNk=";
            }).overrideAttrs
              (
                {
                  preFixup ? "",
                  nativeBuildInputs ? "",
                  ...
                }:
                {

                  nativeBuildInputs = nativeBuildInputs ++ [ self.breakpointHook ];

                  preFixup =
                    preFixup
                    + ''
                      mv "$out/lib64/cmake/"* "$out/lib/cmake"
                      rmdir "$out/lib64/cmake"

                    '';

                }
              );

          geometric-shapes = rosSuper.geometric-shapes.overrideAttrs (
            {
              patches ? [ ],
              ...
            }:
            {
              patches = patches ++ [
                # https://github.com/moveit/geometric_shapes/pull/241
                (self.fetchpatch {
                  url = "https://github.com/moveit/geometric_shapes/commit/78898826b16b7547c69c63ce28b9bddcd167a09e.patch";
                  includes = [ "CMakeLists.txt" ];
                  revert = true;
                  hash = "sha256-elLSrqVnyTyG5P+iPXIx0RccC7TmdPVAZtbhpJcYUO0=";
                })
              ];
            }
          );

          moveit-core = rosSuper.moveit-core.overrideAttrs (
            {
              postPatch ? "",
              ...
            }:
            {
              src = self.fetchzip {
                name = "moveit-core";
                url = "https://github.com/ros2-gbp/moveit2-release/archive/release/jazzy/moveit_core/2.10.0-1.tar.gz";
                sha256 = "sha256-WwWn+S+POgbqVVFiTNS9YCPW4HwH0UtkvCrAYRmEuIE=";
              };
              postPatch =
                postPatch
                + ''
                  substituteInPlace CMakeLists.txt --replace 'find_package(octomap 1.9.7...<1.10.0 REQUIRED)' 'find_package(octomap 1.9.7...1.10.0 REQUIRED)'
                '';
            }
          );

          moveit-ros-occupancy-map-monitor = rosSuper.moveit-ros-occupancy-map-monitor.overrideAttrs (
            {
              postPatch ? "",
              ...
            }:
            {
              src = self.fetchzip {
                name = "ros-jazzy-moveit-ros-occupancy-map-monitor";
                url = "https://github.com/ros2-gbp/moveit2-release/archive/release/jazzy/moveit_ros_occupancy_map_monitor/2.10.0-1.tar.gz";
                hash = "sha256-WHbMOwEkQoPOrHQOeH/0GJyEa7g/ez3LJsJTZw6jUUw=";
              };
              postPatch =
                postPatch
                + ''
                  substituteInPlace CMakeLists.txt --replace 'find_package(octomap 1.9.7...<1.10.0 REQUIRED)' 'find_package(octomap 1.9.7...1.10.0 REQUIRED)'
                '';
            }
          );

          nav2-rviz-plugins = rosSuper.nav2-rviz-plugins.overrideAttrs (
            {
              postPatch ? "",
              ...
            }:
            {
              # remove broken updateAutoDeactivate() symbol, 20/11/2024, Navigation2 1.3.2
              # https://github.com/MonashNovaRover/nova/issues/96
              postPatch =
                postPatch
                + ''
                  substituteInPlace src/costmap_cost_tool.cpp --replace "SLOT(updateAutoDeactivate())" "nullptr"
                  substituteInPlace include/nav2_rviz_plugins/costmap_cost_tool.hpp --replace "private Q_SLOTS:" ""
                  substituteInPlace include/nav2_rviz_plugins/costmap_cost_tool.hpp --replace "void updateAutoDeactivate();" ""
                '';
            }
          );

          controller-manager = rosSuper.controller-manager.overrideAttrs (
            {
              prePatch ? "",
              patches ? [ ],
              ...
            }:
            {
              patches = patches ++ [
                (self.fetchpatch {
                  url = "https://github.com/ros-controls/ros2_control/commit/23bd1c3c06c30d706f010628d85133a7198e226d.patch";
                  hash = "sha256-bM3I5Q4J1DQNJuP2l3mxF7Kh/4DgjjKyRa5FBZS9t9s=";
                  stripLen = 2;
                  extraPrefix = "";
                  excludes = [ "release_notes.rst" ];
                })
              ];

              prePatch =
                prePatch
                + ''
                  pwd
                '';
            }
          );

        }
        // (
          let
            fixRtabmapDependent =
              pkg:
              pkg.overrideAttrs (
                {
                  buildInputs ? [ ],
                  cmakeFlags ? [ ],
                  ...
                }:
                {
                  cmakeFlags = cmakeFlags ++ [ "-DRTABMAP_SYNC_MULTI_RGBD=ON" ];

                  buildInputs =
                    buildInputs
                    ++ (with self; [
                      # For some reason, this is not properly included from rtabmap's
                      # propagatedBuildInputs.
                      (pcl.override { vtk = vtkWithQt5; })
                    ]);

                }
              );
          in
          {
            rtabmap = self.rtabmap.overrideAttrs {
              inherit (rosSuper.rtabmap) pname version src;
            };

            rtabmap-ros = rosSuper.rtabmap-ros.overrideAttrs (
              {
                propagatedBuildInputs ? [ ],
                ...
              }:
              {
                propagatedBuildInputs = self.lib.remove rosSelf.rtabmap-demos propagatedBuildInputs;
              }
            );

            rtabmap-conversions = fixRtabmapDependent rosSuper.rtabmap-conversions;
            rtabmap-odom = fixRtabmapDependent rosSuper.rtabmap-odom;
            rtabmap-rviz-plugins = fixRtabmapDependent rosSuper.rtabmap-rviz-plugins;
            rtabmap-slam = fixRtabmapDependent rosSuper.rtabmap-slam;
            rtabmap-sync = fixRtabmapDependent rosSuper.rtabmap-sync;
            rtabmap-util = fixRtabmapDependent rosSuper.rtabmap-util;
            rtabmap-viz = (fixRtabmapDependent rosSuper.rtabmap-viz).overrideAttrs (
              {
                nativeBuildInputs ? [ ],
                postFixup ? "",
                ...
              }:
              {
                nativeBuildInputs = nativeBuildInputs ++ [ self.qt5.wrapQtAppsHook ];
                postFixup =
                  postFixup
                  + ''
                    wrapQtApp "$out/lib/rtabmap_viz/rtabmap_viz"
                  '';
              }
            );
          }
        )
        // (
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
            cartographer =
              (rosSuper.cartographer.override {
                glog = cartographer-glog;
                ceres-solver = self.ceres-solver.override {
                  glog = cartographer-glog;
                };
              }).overrideAttrs
                (
                  {
                    patches ? [ ],
                    nativeBuildInputs ? [ ],
                    NIX_CFLAGS_COMPILE ? "",
                    ...
                  }:
                  {
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
                  }
                );

            cartographer-ros =
              (rosSuper.cartographer-ros.override {
                glog = cartographer-glog;
              }).overrideAttrs
                (
                  {
                    patches ? [ ],
                    ...
                  }:
                  {
                    patches = patches ++ [
                      # Fix cmake to prevent multiple definitions
                      # https://github.com/ros2/cartographer_ros/pull/63
                      (self.fetchpatch {
                        url = "https://github.com/ros2/cartographer_ros/commit/098e6bf8556070b9228b9fa7d5766238bc141a90.patch";
                        stripLen = 1;
                        hash = "sha256-4hdpQ8LnjT6zuCq/Cr5gACAPpfvCSoGv4BNjERtdpgQ=";
                      })
                    ];
                  }
                );
          }
        )
        // (
          let
            fixNav2Package =
              pkg:
              pkg.overrideAttrs (
                {
                  CXXFLAGS ? "",
                  ...
                }:
                {
                  # https://answers.ros.org/question/379173
                  CXXFLAGS = "${CXXFLAGS} -Wno-error=maybe-uninitialized";
                }
              );
          in
          {
            nav2-behaviors = fixNav2Package rosSuper.nav2-behaviors;
            nav2-constrained-smoother = fixNav2Package rosSuper.nav2-constrained-smoother;
            nav2-planner = fixNav2Package rosSuper.nav2-planner;
            nav2-smoother = fixNav2Package rosSuper.nav2-smoother;
            nav2-waypoint-follower = fixNav2Package rosSuper.nav2-waypoint-follower;
            dwb-critics = fixNav2Package rosSuper.dwb-critics;
            dwb-plugins = fixNav2Package rosSuper.dwb-plugins;
          }
        )
        // (
          let
            replaceUbloxSrc = pkg: pkg.overrideAttrs ({ src, version, ... }: {
              src = self.fetchFromGitHub {
                owner = "vic-bart";
                repo = "ublox_dgnss";
                rev = "1cf2a02a7befd74ba6deb86dd91016cae5dbd690";
                hash = "sha256-AoY48EZjShgXsXXYysGQ5rG+/T5YRXXe8XvDYuqR8XM=";
              };

              version = "0.5.4-r2";
            });
          in
          {
            ntrip-client-node = (replaceUbloxSrc rosSuper.ntrip-client-node).overrideAttrs ({ ... }: {
              sourceRoot = "source/ntrip_client_node";
            });

            ublox-dgnss = (replaceUbloxSrc rosSuper.ublox-dgnss).overrideAttrs ({ ... }: {
              sourceRoot = "source/ublox_dgnss";
            });

            ublox-dgnss-node = (replaceUbloxSrc rosSuper.ublox-dgnss-node).overrideAttrs ({ ... }: {
              sourceRoot = "source/ublox_dgnss_node";
            });

            ublox-nav-sat-fix-hp-node = (replaceUbloxSrc rosSuper.ublox-nav-sat-fix-hp-node).overrideAttrs ({ ... }: {
              sourceRoot = "source/ublox_nav_sat_fix_hp_node";
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
      (
        super.rosPackages
        // {
          foxy = super.rosPackages.foxy.overrideScope (
            rosSelf: rosSuper: {
              # Use ros2doctor from Humble: https://github.com/lopsided98/nix-ros-overlay/issues/75#issuecomment-1567281292
              ros2doctor = rosSelf.callPackage self.rosPackages.humble.ros2doctor.override { };
            }
          );

          humble = super.rosPackages.humble.overrideScope (
            rosSelf: rosSuper: {
              nav2-mppi-controller = rosSuper.nav2-mppi-controller.overrideAttrs (
                {
                  patches ? [ ],
                  ...
                }:
                {
                  patches = patches ++ [
                    # Ignore warnings in included xtensor library
                    # https://github.com/ros-navigation/navigation2/pull/4285
                    (self.fetchpatch {
                      url = "https://github.com/ros-navigation/navigation2/commit/c6ccd8e6db1edc138c6cf3650e192cc595d44e7f.patch";
                      stripLen = 1;
                      hash = "sha256-4g2ESEJz7kuPGT4F0OcAkLbZVJ+84R3NQdpFEZW61Ao=";
                    })
                  ];
                }
              );
            }
          );

          jazzy = super.rosPackages.jazzy.overrideScope (
            rosSelf: rosSuper: {
              # Gazebo Classic is EOL, and the ROS packages have been removed from the
              # distro. The Iron releases still work, though, so add them back.
              gazebo-dev = rosSelf.callPackage (self.nix-ros-overlay + "/distros/iron/gazebo-dev") { };
              gazebo-plugins =
                (rosSelf.callPackage (self.nix-ros-overlay + "/distros/iron/gazebo-plugins") { }).overrideAttrs
                  (
                    {
                      patches ? [ ],
                      ...
                    }:
                    {
                      patches = patches ++ [
                        # Fix deprecation warnings
                        # https://github.com/ros-simulation/gazebo_ros_pkgs/pull/1429
                        (self.fetchpatch {
                          url = "https://github.com/ros-simulation/gazebo_ros_pkgs/commit/4505d7ba69ce1cbf59553d3c499b6f2447cbbbb8.patch";
                          stripLen = 1;
                          includes = [
                            "CMakeLists.txt"
                            "src/**"
                          ];
                          hash = "sha256-JnCbQrhrVl5jKYAmemUFk+u0W+ByCG/QJPMGAFAxGkA=";
                        })
                      ];
                    }
                  );
              gazebo-ros =
                (rosSelf.callPackage (self.nix-ros-overlay + "/distros/iron/gazebo-ros") { }).overrideAttrs
                  (
                    {
                      patches ? [ ],
                      ...
                    }:
                    {
                      patches = patches ++ [
                        # Fix deprecation warnings
                        # https://github.com/ros-simulation/gazebo_ros_pkgs/pull/1429
                        (self.fetchpatch {
                          url = "https://github.com/ros-simulation/gazebo_ros_pkgs/commit/4505d7ba69ce1cbf59553d3c499b6f2447cbbbb8.patch";
                          stripLen = 1;
                          includes = [ "**.py" ];
                          hash = "sha256-2l6Ft+3F7dskjjOpTeQj202AMnEjQSF+h9j81rxrqzk=";
                        })
                      ];
                    }
                  );
              gazebo-ros2-control =
                (rosSelf.callPackage (self.nix-ros-overlay + "/distros/iron/gazebo-ros2-control") { }).overrideAttrs
                  rec {
                    version = "0.7.2";
                    src = self.fetchFromGitHub {
                      owner = "ros-controls";
                      repo = "gazebo_ros2_control";
                      rev = version;
                      hash = "sha256-ya+HFf6qOGMVpKOVWlv8+Kp3h/G011MZA+Wnrztq3zg=";
                    };
                    sourceRoot = src.name + "/gazebo_ros2_control";
                  };
              gazebo-ros-pkgs = rosSelf.callPackage (self.nix-ros-overlay + "/distros/iron/gazebo-ros-pkgs") { };

              image-proc = rosSuper.image-proc.overrideAttrs (
                {
                  patches ? [ ],
                  ...
                }:
                {
                  patches = patches ++ [
                    ./patches/image_proc.patch
                  ];
                }
              );

              gz-sensors-vendor = (
                rosSelf.lib.patchAmentVendorGit rosSuper.gz-sensors-vendor {
                  url = "https://github.com/gazebosim/gz-sensors";
                  rev = "gz-sensors8_8.2.1";
                  fetchgitArgs.hash = "sha256-wEUJoHbvvImuFbaKk84maw5AoKhoEhdU0uOYVBtHhI0=";
                }
              );

              rosapi = rosSuper.rosapi.overrideAttrs (
                {
                  patches ? [ ],
                  ...
                }:
                {
                  patches = patches ++ [
                    # Fix invalid import of get_parameter_value in rosapi for ROS2 Jazzy.
                    # https://github.com/RobotWebTools/rosbridge_suite/pull/932
                    (self.fetchpatch {
                      url = "https://github.com/RobotWebTools/rosbridge_suite/commit/d22f102b59e7d9fdeea0ec5e74aa8b98358585d7.patch";
                      stripLen = 1;
                      hash = "sha256-zmRHt7EgZk8kF2Dv1+QvTmox47RR7TBZOOdKfnIySog=";
                    })
                  ];
                }
              );

              robot-localization = rosSuper.robot-localization.overrideAttrs (
                {
                  patches ? [ ],
                  ...
                }:
                {
                  patches = patches ++ [
                    # Add stamped_control as a parameter to support TwistStamped msgs.
                    # https://github.com/cra-ros-pkg/robot_localization/pull/900
                    (self.fetchpatch {
                      url = "https://patch-diff.githubusercontent.com/raw/cra-ros-pkg/robot_localization/pull/900.patch";
                      stripLen = 1;
                      extraPrefix = "";
                      hash = "sha256-Wh3WOLYYHGL25eFRqpGUbvosle2QW6g7OIhXrZ8EG6A=";
                    })
                  ];
                }
              );
            }
          );
        }
      )
  );

  # Overlays for non-ROS packages
  cudaPackages = super.cudaPackages // {
    nvidia-optical-flow-sdk = super.nvidia-optical-flow-sdk;
  };
}
