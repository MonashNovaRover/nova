self: super:

{
  rosPackages =
    (super.rosPackages.appendDistroOverlay
      # Overlay for all ROS distros.
      (rosSelf: rosSuper: {
        ## ROSBRIDGE

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

        ## NAV2
        nav2-core = rosSuper.nav2-core.overrideAttrs ({ patches ? [ ], ... }: {
          patchFlags = [ "-p2" ];
          patches = patches ++ [
            # Make nav2_core an INTERFACE library
            # https://github.com/ros-navigation/navigation2/pull/4578
            (self.fetchpatch {
              url = "https://github.com/ros-navigation/navigation2/commit/a001bfc3c5fc4fdea026cb6599175fa7b429306c.diff";
              includes = [ "nav2_core/**" ];
              hash = "sha256-dwY3miREtSfjDx92tSXkqdhgeFdDYEqzYqGG0uD44kI=";
            })
          ];
        });

        nav2-behavior-tree = rosSuper.nav2-behavior-tree.overrideAttrs ({ patches ? [ ], nativeBuildInputs ? [ ], ... }: {
          patchFlags = [ "-p2" ];
          nativeBuildInputs = nativeBuildInputs ++ [ self.breakpointHook ];
          patches = patches ++ [
            # Remove temp BT.CPP build warning workaround
            # https://github.com/ros-navigation/navigation2/pull/4500
            (self.fetchpatch {
              url = "https://github.com/ros-navigation/navigation2/commit/1d60b16fa848201ff703f7e8939d3f3a043a5ecd.diff";
              hash = "sha256-L1IA9uo1DRaC6dDM2X72CkZallz7Co9yBzWMjqzPDck=";
            })

            # Revamp nav2_behavior_tree CMakeLists.txt to use modern idioms
            # https://github.com/ros-navigation/navigation2/pull/4485
            # (self.fetchpatch {
            #   url = "https://github.com/ros-navigation/navigation2/commit/ba247b473ca1dea4f4ceac06532f6332250615b0.diff";
            #   hash = "sha256-0BmBqK5gydWw5mzbXTgo2spcLgRZ/vCrIwrFETbt8EA=";
            # })
            # Leaving the above in case we need to refer to it again.
            # This custom patch file was written because the above patch was missing
            # some lines from the CMakeLists.txt:
            # install(DIRECTORY test/utils/
            #   DESTINATION include/${PROJECT_NAME}/nav2_behavior_tree/test/utils
            # )
            ./patches/nav2-behavior-tree.patch
          ];
        });

        # nav2_rviz_plugins: Add route_tool
        nav2-rviz-plugins =
          rosSuper.nav2-rviz-plugins.overrideAttrs (old: {
            patchFlags = [ "-p1" ];

            patches = (old.patches or []) ++ [
              # Backport of nav2_rviz_plugins::RouteTool plugin to Jazzy
              ./patches/nav2-route-tool.patch
            ];

            buildInputs = (old.buildInputs or []) ++ [
              rosSuper.nav2-route
            ];

            propagatedBuildInputs = (old.propagatedBuildInputs or []) ++ [
              rosSuper.nav2-route
            ];
        });

        # nav2-smoother = rosSuper.nav2-smoother.overrideAttrs ({ patches ? [ ], ... }: {
        #   patchFlags = [ "-p2" ];
        #   patches = patches ++ [
        #     # Fixing and refactoring Sovitsky-Golay Filter in MPPI and Smoother
        #     # https://github.com/ros-navigation/navigation2/pull/4669
        #     (self.fetchpatch {
        #       url = "https://github.com/ros-navigation/navigation2/commit/1a3b637d90484a3d6cac5e2cd75836df8518a32b.diff";
        #       includes = [ "nav2_smoother/**" ];
        #       hash = "sha256-zeqbMHMqgLPEPKiwaoieFNobhTPGA9buGmvIj2SBHc4=";
        #     })
        #   ];
        # });

        nav2-mppi-controller = rosSuper.nav2-mppi-controller.overrideAttrs ({ patches ? [ ], ... }: {
          patchFlags = [ "-p2" ];

          patches = patches ++ [
            # Fix goal pose stamp (backport #4854) 
            # https://github.com/ros-navigation/navigation2/pull/4855 
            (self.fetchpatch { 
              url = "https://github.com/ros-navigation/navigation2/commit/65eab414f3ddeccceb988e89af91e645c48d06d6.diff"; 
              revert = true; 
              hash = "sha256-r5fag+emiM/6qwVgLSDxkVgKvpwuGzKDe8Oh/XfyueM="; 
            }) 
            
            # Mppi goal to critic (backport #4822) 
            # https://github.com/ros-navigation/navigation2/pull/4853 
            (self.fetchpatch { 
              url = "https://github.com/ros-navigation/navigation2/commit/a6a4c26348efc6d7e265a5080d9165a4af699337.diff"; 
              revert = true; 
              hash = "sha256-VJveHIlHiAfOZVkNZ2oWB246xRLpqY7fy7eLLFNQjtQ="; 
            })

            # Backport bidirectional settings #4954 to Jazzy
            # https://github.com/ros-navigation/navigation2/pull/5260 
            (self.fetchpatch { 
              url = "https://github.com/ros-navigation/navigation2/commit/b47bcfa5e633e4c06b7def8a615e16cfdbec397c.diff";
              revert = true; 
              excludes = [ "nav2_bringup/params/nav2_params.yaml" ];
              hash = "sha256-kl6YqprEbUUJXq05C9CxOMZ7CQ6gMoTcLcimAPAvoNI="; 
            }) 

            # mppi parameters_handler: Improve verbose handling (#4704)
            # https://github.com/ros-navigation/navigation2/pull/4711
            (self.fetchpatch {
              url = "https://github.com/ros-navigation/navigation2/commit/f45d05b809064f27ecd9d141043a01fd8b41f063.diff";
              hash = "sha256-R9TVmtfajtIuGWIMcFG9oQSnd5++OBqNyFsR75kqJwA=";
            })

            # Fix typos
            # https://github.com/ros-navigation/navigation2/pull/4796
            (self.fetchpatch {
              url = "https://github.com/ros-navigation/navigation2/commit/9d60bc0a3ca4e201250254c866c4eedc1441ed4e.diff";
              includes = [ "nav2_mppi_controller/**" ];
              hash = "sha256-toGRnHr7MZxPCw/nvchiYmX2Nj/dG3Nrc5sG3Ba+De0=";
            })

            # Switch nav2_mppi_controller to modern CMake idioms
            # https://github.com/ros-navigation/navigation2/pull/4590
            (self.fetchpatch {
              url = "https://github.com/ros-navigation/navigation2/commit/d64ff6bff12b436af0a38294d068a633cba1b231.diff";
              hash = "sha256-msErSoWV73g9YLlAAa8SLhqmfgjsVSrThxFsgzYYt0s=";
            })

            # Various small CMake fixes
            # https://github.com/ros-navigation/navigation2/pull/4643
            (self.fetchpatch {
              url = "https://github.com/ros-navigation/navigation2/commit/c38fefd45531af496099d129b9ad5b8f407161be.diff";
              includes = [ "nav2_mppi_controller/**" ];
              hash = "sha256-K2vohrP+R1dMwvhs9XFcvWW9msuEG3bEMTtO8jK7F9A=";
            })

            # Mppi goal to critic
            # https://github.com/ros-navigation/navigation2/pull/4822
            (self.fetchpatch {
              url = "https://github.com/ros-navigation/navigation2/commit/d11de56438da34d55175e15cacd72fa236c9fff6.diff";
              hash = "sha256-B9zsBsXLGyPyREuocTVWOQkdt9C2ma//QRrG8ieTUuw=";
            })

            # Fix goal pose stamp
            # https://github.com/ros-navigation/navigation2/pull/4854
            (self.fetchpatch {
              url = "https://github.com/ros-navigation/navigation2/commit/694a222edf7c765c9691fdd10a8025ed6b4aa8ce.diff";
              hash = "sha256-X0ncI0CNi4xRxUL6CFzuVGFh1GHGTJ9+AcZ5SzLJWRk=";
            })

            # 45-50% performance improvement in MPPI controller using Eigen library for computation
            # https://github.com/ros-navigation/navigation2/pull/4621
            # Fixes https://github.com/ros-navigation/navigation2/issues/4380
            (self.fetchpatch {
              url = "https://github.com/ros-navigation/navigation2/commit/a33e8d2bef0984c3eef8a03c7f05dc69704225c7.diff";
              hash = "sha256-FISECyT0ZaSHJ10sqjrvawD0A6UAQg+0ASwzT9sdefg=";
            })
          ];

          nativeBuildInputs = with rosSelf; [ ament-cmake ];

          buildInputs = with self; with rosSelf; [
            angles
            geometry-msgs
            llvmPackages.openmp
            nav2-common
            nav2-core
            nav2-costmap-2d
            nav2-util
            nav2-msgs
            pluginlib
            rclcpp
            rclcpp-lifecycle
            std-msgs
            tf2
            tf2-geometry-msgs
            tf2-ros
            visualization-msgs
            eigen3-cmake-module
            eigen
          ];

          propagatedBuildInputs = [ ];
        });
      }))

      # Overlays for individual ROS distros.
      (super.rosPackages // { });
}
