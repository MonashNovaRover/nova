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
            # NOTE: hunk #2 of optimizer.cpp fails because context changed upstream.
            # The failed hunk is applied by postPatch below.
            (self.fetchpatch { 
              url = "https://github.com/ros-navigation/navigation2/commit/a6a4c26348efc6d7e265a5080d9165a4af699337.diff"; 
              revert = true; 
              hash = "sha256-VJveHIlHiAfOZVkNZ2oWB246xRLpqY7fy7eLLFNQjtQ="; 
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

            # Optimize MPPI visualization to skip processing when no subscribers
            # https://github.com/ros-navigation/navigation2/pull/5807
            # custom patch needed because of conflicts with previous Eigen patch
            ./patches/mppi-visualisation.patch
          ];

          # The a6a4c263 revert fetchpatch partially fails (hunk #2 of optimizer.cpp
          # has wrong context due to upstream changes). Override patchPhase to tolerate
          # partial failures, then use postPatch to apply the failed hunk manually.
          patchPhase = ''
            runHook prePatch
            for patch in $patches; do
              echo "Applying patch $patch"
              patch -p2 --batch < "$patch" || echo "WARN: patch had partial failures, continuing..."
            done
            runHook postPatch
          '';

          postPatch = ''
            # Fix the failed hunk #2 from a6a4c263 revert: remove goal param from prepare()
            # The revert removed goal from evalControl (hunk #1 succeeded) but failed on
            # prepare (hunk #2) because context line state_.speed = robot_speed doesn't match.
            sed -i '/^  const geometry_msgs::msg::Pose & goal,$/d' src/optimizer.cpp
            sed -i '/^  const nav_msgs::msg::Path & plan,$/{N;s|\n  nav2_core::GoalChecker \* goal_checker)|, nav2_core::GoalChecker * goal_checker)|}' src/optimizer.cpp
            sed -i '/^  goal_ = goal;$/d' src/optimizer.cpp

            # Write the entire CMakeLists.txt from scratch. Too many patches partially
            # fail on this file leaving it in an inconsistent state (xtensor refs mixed
            # with Eigen, broken exports, etc). The target state incorporates:
            # - Eigen optimization (replaces xtensor/xsimd)
            # - CMake modernization (per-target options instead of foreach loop)
            # - Small CMake fixes
            cat > CMakeLists.txt << 'CMAKE_EOF'
            cmake_minimum_required(VERSION 3.5)
            project(nav2_mppi_controller)

            find_package(ament_cmake REQUIRED)
            find_package(angles REQUIRED)
            find_package(eigen3_cmake_module REQUIRED)
            find_package(Eigen3 REQUIRED)
            find_package(geometry_msgs REQUIRED)
            find_package(nav2_common REQUIRED)
            find_package(nav2_core REQUIRED)
            find_package(nav2_costmap_2d REQUIRED)
            find_package(nav2_msgs REQUIRED)
            find_package(nav2_params REQUIRED)
            find_package(nav2_util REQUIRED)
            find_package(nav_msgs REQUIRED)
            find_package(pluginlib REQUIRED)
            find_package(rclcpp REQUIRED)
            find_package(tf2 REQUIRED)
            find_package(tf2_eigen REQUIRED)
            find_package(tf2_geometry_msgs REQUIRED)
            find_package(tf2_ros REQUIRED)
            find_package(visualization_msgs REQUIRED)

            include_directories(
              include
              ${EIGEN3_INCLUDE_DIR}
            )

            nav2_package()

            include(CheckCXXCompilerFlag)
            check_cxx_compiler_flag("-mfma" COMPILER_SUPPORTS_FMA)
            if(COMPILER_SUPPORTS_FMA)
              add_compile_options(-mfma)
            endif()

            add_library(mppi_controller SHARED
              src/controller.cpp
              src/critic_manager.cpp
              src/noise_generator.cpp
              src/optimizer.cpp
              src/parameters_handler.cpp
              src/path_handler.cpp
              src/trajectory_visualizer.cpp
            )
            target_compile_options(mppi_controller PUBLIC -O3)
            target_include_directories(mppi_controller
              PUBLIC
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                "$<INSTALL_INTERFACE:include/${PROJECT_NAME}>")
            target_link_libraries(mppi_controller PUBLIC
              angles::angles
              Eigen3::Eigen
              geometry_msgs::geometry_msgs
              nav2_common::nav2_common
              nav2_core::nav2_core
              nav2_costmap_2d::nav2_costmap_2d
              nav2_msgs::nav2_msgs
              nav2_util::nav2_util
              nav_msgs::nav_msgs
              rclcpp::rclcpp
              tf2::tf2
              tf2_eigen::tf2_eigen
              tf2_geometry_msgs::tf2_geometry_msgs
              tf2_ros::tf2_ros
              ${visualization_msgs_TARGETS}
            )

            add_library(mppi_critics SHARED
              src/critics/constraint_critic.cpp
              src/critics/cost_critic.cpp
              src/critics/goal_angle_critic.cpp
              src/critics/goal_critic.cpp
              src/critics/obstacles_critic.cpp
              src/critics/path_align_critic.cpp
              src/critics/path_angle_critic.cpp
              src/critics/path_follow_critic.cpp
              src/critics/prefer_forward_critic.cpp
              src/critics/twirling_critic.cpp
              src/critics/velocity_deadband_critic.cpp
            )
            target_compile_options(mppi_critics PUBLIC -fconcepts -O3)
            target_include_directories(mppi_critics
              PUBLIC
                "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
                "$<INSTALL_INTERFACE:include/${PROJECT_NAME}>")
            target_link_libraries(mppi_critics PUBLIC
              angles::angles
              Eigen3::Eigen
              geometry_msgs::geometry_msgs
              nav2_common::nav2_common
              nav2_core::nav2_core
              nav2_costmap_2d::nav2_costmap_2d
              nav2_msgs::nav2_msgs
              nav2_util::nav2_util
              nav_msgs::nav_msgs
              rclcpp::rclcpp
              tf2::tf2
              tf2_eigen::tf2_eigen
              tf2_geometry_msgs::tf2_geometry_msgs
              tf2_ros::tf2_ros
              ${visualization_msgs_TARGETS}
            )
            target_link_libraries(mppi_critics PRIVATE
              pluginlib::pluginlib
            )

            install(TARGETS mppi_controller mppi_critics
              EXPORT nav2_mppi_controller
              ARCHIVE DESTINATION lib
              LIBRARY DESTINATION lib
              RUNTIME DESTINATION bin
            )

            install(DIRECTORY include/
              DESTINATION include/${PROJECT_NAME}
            )

            if(BUILD_TESTING)
              find_package(ament_lint_auto REQUIRED)
              set(ament_cmake_copyright_FOUND TRUE)
              ament_lint_auto_find_test_dependencies()
              find_package(ament_cmake_gtest REQUIRED)
              ament_find_gtest()
              add_subdirectory(test)
            endif()

            ament_export_libraries(mppi_controller mppi_critics)
            ament_export_dependencies(
              angles
              geometry_msgs
              nav2_common
              nav2_core
              nav2_costmap_2d
              nav2_msgs
              nav2_util
              nav_msgs
              rclcpp
              tf2
              tf2_geometry_msgs
              tf2_ros
              visualization_msgs
              Eigen3
            )
            ament_export_include_directories(include/${PROJECT_NAME})
            ament_export_targets(nav2_mppi_controller)
            pluginlib_export_plugin_description_file(nav2_core mppic.xml)
            pluginlib_export_plugin_description_file(nav2_mppi_controller critics.xml)

            ament_package()
            CMAKE_EOF
          '';

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
