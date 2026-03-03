
self: super:

{
  rosPackages = (
    super.rosPackages.appendDistroOverlay
      # Overlay for all ROS distros.
      (
        rosSelf: rosSuper:
        {
          usb-cam = rosSuper.usb-cam.overrideAttrs (
            {
              postPatch ? [ ],
              ...
            }:
            {
              postPatch = postPatch ++ [
                "sed -i '/AV_PIX_FMT_XVMC/d' include/usb_cam/formats/av_pixel_format_helper.hpp"
              ];
            }
          );

          librealsense2 = rosSuper.librealsense2.overrideAttrs (
            {
              cmakeFlags ? [],
              patches ? [],
              ...
            }:
            {
              cmakeFlags = cmakeFlags ++ [
                "-DCHECK_FOR_UPDATES=OFF"
              ];

              patches = patches ++ [
                ./patches/librealsense2.patch
              ];
            }
          );

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
            rtabmap = self.rtabmap.overrideAttrs (
              {
                patches ? [], 
                propagatedBuildInputs ? [], 
                ...
            }: 
            {
              inherit (rosSuper.rtabmap) pname;
              version = "0.21.10-r1";
              src = self.fetchurl {
                url = "https://github.com/ros2-gbp/rtabmap-release/archive/release/jazzy/rtabmap/0.21.10-1.tar.gz";
                name = "0.21.10-1.tar.gz";
                sha256 = "sha256-qT2xYc1I/J0sWffxH1yOtYJV9h6sc1SybI2t2YoGb+I=";
              };
              propagatedBuildInputs = propagatedBuildInputs ++ [ self.qt5.wrapQtAppsHook self.librealsense self.octomap ];
              patches = patches ++ [
                # Fix compilation with boost >= 1.87
                (self.fetchpatch {
                  url = "https://github.com/introlab/rtabmap/commit/08f031e11c45589fc2b68440383a3e40982dc06f.patch";
                  hash = "sha256-avU8I19qHFcKBBdIsE4rPJZIHwSy4Wssmwt10cPmk6k=";
                })
              ];
            });

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
                  CXXFLAGS = "${CXXFLAGS} -Wno-error=maybe-uninitialized -Wno-error=array-bounds";
                  
                }
                
              );
          in
          {
            nav2-behaviors = fixNav2Package rosSuper.nav2-behaviors;
            nav2-constrained-smoother = fixNav2Package rosSuper.nav2-constrained-smoother;
            nav2-costmap-2d = fixNav2Package rosSuper.nav2-costmap-2d;
            nav2-planner = fixNav2Package rosSuper.nav2-planner;
            nav2-smoother = fixNav2Package rosSuper.nav2-smoother;
            nav2-waypoint-follower = fixNav2Package rosSuper.nav2-waypoint-follower;
            dwb-critics = fixNav2Package rosSuper.dwb-critics;
            dwb-plugins = fixNav2Package rosSuper.dwb-plugins;
          }
        )
      )

      # Overlays for individual ROS distros.
      (
        super.rosPackages
        // {
          jazzy = super.rosPackages.jazzy.overrideScope (
            rosSelf: rosSuper: 
            let
              gz-msgs-source = self.fetchgit {
                url = "https://github.com/gazebosim/gz-msgs.git";
                rev = "gz-msgs10_10.3.2";
                name = "gz-msgs10_10.3.2";
                sha256 = "sha256-lVQ7azT/HesBi3bnk8E5jFYORZh7jAIk6vDK7BcT0Ds=";
                postFetch = ''
                  cd $out
                  patch -p1 < ${self.fetchpatch {
                    url = "https://github.com/gazebosim/gz-msgs/commit/22c57006798470db63e8ecaff7b49dce34d5e76f.patch";
                    hash = "sha256-Qkf3JgN8twh6fRLblZj9NmsOxT6jTBgCV1SDJHUk3+w=";
                  }}
                '';
              };
              gz-msgs-tarball = rosSelf.lib.tarSource {} gz-msgs-source;

              gz-transport-source = self.fetchgit {
                url = "https://github.com/gazebosim/gz-transport.git";
                rev = "gz-transport13_13.4.1";
                name = "gz-transport13_13.4.1";
                sha256 = "sha256-flyDskV+FD1tMSnVLFEqf1e/tbgVuZjqPpF/M3jVFyU=";
                postFetch = ''
                  cd $out
                  patch -p1 <  ${./patches/gz-transport.patch}
                '';
              };
              gz-transport-tarball = rosSelf.lib.tarSource {} gz-transport-source;
              
              gz-gui-source = self.fetchgit {
                url = "https://github.com/gazebosim/gz-gui.git";
                rev = "gz-gui8_8.4.0";
                name = "gz-gui8_8.4.0";
                sha256 = "sha256-JT3eq0HG9OnSQhmPvXMd78w7xnVliosHNZ0I1TMpfjY=";
                postFetch = ''
                  cd $out
                  patch -p1 < ${self.fetchpatch {
                    url = "https://patch-diff.githubusercontent.com/raw/gazebosim/gz-gui/pull/677.patch";
                    hash = "sha256-9nX3/Yyxp5WSE8VvY+TWcfPFNlS8pdbtex0mujqiilw=";
                  }}
                '';
              };
              gz-gui-tarball = rosSelf.lib.tarSource {} gz-gui-source;

              gz-sim-source = self.fetchgit {
                url = "https://github.com/gazebosim/gz-sim.git";
                rev = "gz-sim8_8.9.0";
                name = "gz-sim8_8.9.0";
                sha256 = "sha256-ZzInawfKfuIZ/YsE4ogkBBKzAg8T7bMj/dbo8Hy+hKU=";
                postFetch = ''
                  cd $out
                  patch -p1 <  ${./patches/gz-sim.patch}
                '';
              };
              gz-sim-tarball = rosSelf.lib.tarSource {} gz-sim-source;
            in
            {
              joint-limits = rosSuper.joint-limits.overrideAttrs (
                {
                  patches ? [ ],
                  ...
                }:
                {
                  patches = patches ++ [
                    ./patches/joint-limits-saturation-velocity.patch
                  ];
                }
              );

              gz-msgs-vendor = rosSuper.gz-msgs-vendor.overrideAttrs (
                {
                  postPatch ? "",
                  ...
                }:
                {
                  postPatch = postPatch + ''
                    sed -i 's|file:///nix/store/[^"]*gz-msgs10_10\.3\.2\.tar|file://${gz-msgs-tarball}|' CMakeLists.txt
                  ''; 
                }
              );

              gz-transport-vendor = rosSuper.gz-transport-vendor.overrideAttrs (
                {
                  postPatch ? "",
                  ...
                }:
                {
                  postPatch = postPatch + ''
                    sed -i 's|file:///nix/store/[^"]*gz-transport13_13\.4\.1\.tar|file://${gz-transport-tarball}|' CMakeLists.txt
                  ''; 
                }
              );

              gz-gui-vendor = rosSuper.gz-gui-vendor.overrideAttrs (
                {
                  postPatch ? "",
                  ...
                }:
                {
                  postPatch = postPatch + ''
                    sed -i 's|file:///nix/store/[^"]*gz-gui8_8\.4\.0\.tar|file://${gz-gui-tarball}|' CMakeLists.txt
                  ''; 
                }
              );

              gz-sim-vendor = rosSuper.gz-sim-vendor.overrideAttrs (
                {
                  postPatch ? "",
                  ...
                }:
                {
                  postPatch = postPatch + ''
                    sed -i 's|file:///nix/store/[^"]*gz-sim8_8\.9\.0\.tar|file://${gz-sim-tarball}|' CMakeLists.txt
                  ''; 
                }
              );

              ntrip-client-node = rosSuper.ntrip-client-node.overrideAttrs rec {
                version = "0.5.5-r3";
                src = self.fetchurl {
                  url = "https://github.com/ros2-gbp/ublox_dgnss-release/archive/release/jazzy/ntrip_client_node/0.5.5-3.tar.gz";
                  name = "0.5.5-3.tar.gz";
                  sha256 = "sha256-dV2nf44RBrq0S3a74hr7j1Fe0zSnRD8jYf41QhgeuuM=";
                };
              };

              ublox-dgnss-node = rosSuper.ublox-dgnss-node.overrideAttrs rec {
                  version = "0.5.5-r3";
                  src = self.fetchurl {
                    url = "https://github.com/ros2-gbp/ublox_dgnss-release/archive/release/jazzy/ublox_dgnss_node/0.5.5-3.tar.gz";
                    name = "0.5.5-3.tar.gz";
                    sha256 = "sha256-V7XuNOmFnwo3yxYnEy6w+a7Q/iLUxaZ7xa0vPV09XdY=";
                  };
              };

              ublox-dgnss = rosSuper.ublox-dgnss.overrideAttrs rec {
                version = "0.5.5-r3";
                src = self.fetchurl {
                  url = "https://github.com/ros2-gbp/ublox_dgnss-release/archive/release/jazzy/ublox_dgnss/0.5.5-3.tar.gz";
                  name = "0.5.5-3.tar.gz";
                  sha256 = "sha256-mzmcFu6UvnE/q096viexA0YgQNHD5A9ZgCVEFdEQMOg=";
                };
              };

              ublox-nav-sat-fix-hp-node = rosSuper.ublox-nav-sat-fix-hp-node.overrideAttrs rec {
                version = "0.5.5-r3";
                src = self.fetchurl {
                  url = "https://github.com/ros2-gbp/ublox_dgnss-release/archive/release/jazzy/ublox_nav_sat_fix_hp_node/0.5.5-3.tar.gz";
                  name = "0.5.5-3.tar.gz";
                  sha256 = "sha256-IxRmU6bggmpo322oHN9F7g+GnKKfH3Yk66u2HxCk6es=";
                };
              };

              ublox-ubx-interfaces = rosSuper.ublox-ubx-interfaces.overrideAttrs rec {
                version = "0.5.5-r3";
                src = self.fetchurl {
                  url = "https://github.com/ros2-gbp/ublox_dgnss-release/archive/release/jazzy/ublox_ubx_interfaces/0.5.5-3.tar.gz";
                  name = "0.5.5-3.tar.gz";
                  sha256 = "sha256-uGJKsL/vscgyl3MqjaXFOx0EEB3AMtadU8wYbsmwB0M=";
                };
              };

              ublox-ubx-msgs = rosSuper.ublox-ubx-msgs.overrideAttrs rec {
                version = "0.5.5-r3";
                src = self.fetchurl {
                  url = "https://github.com/ros2-gbp/ublox_dgnss-release/archive/release/jazzy/ublox_ubx_msgs/0.5.5-3.tar.gz";
                  name = "0.5.5-3.tar.gz";
                  sha256 = "sha256-agwbwcFQv0MlgSdL8gSez1+8GHIF8d99yOoy1RAVCsE=";
                };
              };

              nav2-core = rosSuper.nav2-core.overrideAttrs
              {
                version = "1.3.11-r1";
                src = self.fetchurl {
                  url = "https://github.com/SteveMacenski/navigation2-release/archive/release/jazzy/nav2_core/1.3.11-1.tar.gz";
                  name = "1.3.11-1.tar.gz";
                  sha256 = "868d901037a294caaf9aa3df96f2f909e2625cae88a8f8c203145f1df98b1a64";
                };
              };

              nav2-behavior-tree = rosSuper.nav2-behavior-tree.overrideAttrs
              {
                version = "1.3.11-r1";
                src = self.fetchurl {
                  url = "https://github.com/SteveMacenski/navigation2-release/archive/release/jazzy/nav2_behavior_tree/1.3.11-1.tar.gz";
                  name = "1.3.11-1.tar.gz";
                  sha256 = "4103ab7588bcbfbbd7f03a1d35a92630141b37af85d65d6fcb1ae0191a26b9e0";
                };
              };

              nav2-msgs = rosSuper.nav2-msgs.overrideAttrs
              {
                version = "1.3.11-r1";
                src = self.fetchurl {
                  url = "https://github.com/SteveMacenski/navigation2-release/archive/release/jazzy/nav2_msgs/1.3.11-1.tar.gz";
                  name = "1.3.11-1.tar.gz";
                  sha256 = "0829be46734689ff6e325d5f3701d96e9fe68371261b49c5622221b511f05789";
                };
              };

              nav2-rviz-plugins = rosSuper.nav2-rviz-plugins.overrideAttrs
              (
                {
                  propagatedBuildInputs ? [ ], ...
                }:
                {
                  version = "1.3.11-r1";
                  src = self.fetchurl {
                    url = "https://github.com/SteveMacenski/navigation2-release/archive/release/jazzy/nav2_rviz_plugins/1.3.11-1.tar.gz";
                    name = "1.3.11-1.tar.gz";
                    sha256 = "aa446a165ce5a23952a1e2637964e07f43df8a54d71f473094d59b509d28b668";
                  };
                  propagatedBuildInputs = propagatedBuildInputs ++ ( with rosSuper; [
                    nav2-route
                  ]);
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