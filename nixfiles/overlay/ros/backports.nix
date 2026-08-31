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

            # Fix incomplete xtensor-to-Eigen migration in motion_models.hpp.
            # The Eigen optimization patch (a33e8d2b) partially failed to apply to this file.
            # The types were converted from xtensor to Eigen, but .shape() calls (xtensor method)
            # were not converted to .rows()/.cols() (Eigen method). This error cascades to all
            # 16 translation units that include this header.
            sed -i 's/\.shape(0)/.rows()/g; s/\.shape(1)/.cols()/g' include/nav2_mppi_controller/motion_models.hpp

            # Fix incomplete xtensor-to-Eigen migration in trajectory_visualizer.cpp.
            # The Eigen optimization patch (a33e8d2b) failed to fully convert this file:
            # the xtensor type xt::xtensor<float,2> was not replaced with Eigen::ArrayXXf,
            # and .shape()/.shape[n] calls were not replaced with .rows()/.cols()/.
            python3 << 'PYEOF'
            import re, sys
            with open('src/trajectory_visualizer.cpp', 'r') as f:
                content = f.read()
            content = content.replace('xt::xtensor<float, 2>', 'Eigen::ArrayXXf')
            content = content.replace('auto & size = trajectory.shape()[0]', 'size_t size = trajectory.rows()')
            # Replace .shape() with .rows() for zero-arg case
            content = re.sub(r'\.shape\(\)', '.rows()', content)
            # Replace .shape(0) and .shape(1) with .rows() and .cols()
            content = re.sub(r'\.shape\(\s*0\s*\)', '.rows()', content)
            content = re.sub(r'\.shape\(\s*1\s*\)', '.cols()', content)
            content = content.replace('.shape()[0]', '.rows()')
            content = content.replace('.shape()[1]', '.cols()')
            # Replace 'auto & shape = trajectories.x.rows()' with n_rows/n_cols definitions
            content = re.sub(
                r'auto\s+&\s+shape\s*=\s*trajectories\.x\.rows\(\)\s*;',
                'size_t n_rows = trajectories.x.rows();\n  size_t n_cols = trajectories.x.cols();',
                content)
            # Replace shape[0] and shape[1] with n_rows and n_cols
            content = content.replace('shape[0]', 'n_rows')
            content = content.replace('shape[1]', 'n_cols')
            with open('src/trajectory_visualizer.cpp', 'w') as f:
                f.write(content)
            PYEOF

            # Fix incomplete xtensor-to-Eigen migration in goal_angle_critic.cpp.
            # The Eigen optimization patch (a33e8d2b) failed to apply hunk #1 to this file,
            # so the xtensor->Eigen conversion was never applied.
            python3 << 'PYEOF'
            import re
            with open('src/critics/goal_angle_critic.cpp', 'r') as f:
                content = f.read()

            # Fix .shape(0) -> .size() for path length
            content = content.replace('data.path.x.shape(0)', 'data.path.x.size()')

            # Replace xt::eval() calls - just remove xt::eval wrapper
            content = re.sub(r'xt::eval\(([^)]+)\)', r'\1', content)

            # Replace xt::fabs(x) -> (x).abs()
            content = re.sub(r'xt::fabs\(([^)]+)\)', r'(\1).abs()', content)

            # Replace xt::pow(x, y) -> (x).pow(y) - handles chained .pow too
            content = re.sub(r'xt::pow\(([^,]+),\s*([^)]+)\)', r'(\1).pow(\2)', content)

            # Replace xt::mean(x, {1}) or xt::mean(x) -> (x).rowwise().mean()
            content = re.sub(r'xt::mean\(([^,)]+)(?:,\s*\{1\})?\)', r'(\1).rowwise().mean()', content)

            # Replace xt::minimum(a, b) -> a.cwiseMin(b)
            content = re.sub(r'xt::minimum\(([^,]+),\s*([^)]+)\)', r'(\1).cwiseMin(\2)', content)

            # Force all auto angular/symmetric_distance declarations to be evaluated Arrays
            content = re.sub(
                r'(auto\s+angular_distances\s*=\s*[^;]+\.abs\(\))\s*;',
                r'\1.eval();',
                content)
            content = re.sub(
                r'(auto\s+symmetric_distances\s*=\s*[^;]+\.abs\(\))\s*;',
                r'\1.eval();',
                content)
            # Also handle any auto angular/symmetric_distances without .abs()
            content = re.sub(
                r'(auto\s+angular_distances\s*=\s*utils::[^(]+\([^)]*\))\s*;',
                r'\1.eval();',
                content)
            content = re.sub(
                r'(auto\s+symmetric_distances\s*=\s*utils::[^(]+\([^)]*\))\s*;',
                r'\1.eval();',
                content)
            # Directly fix the cwiseMin assignment to use .eval()
            content = re.sub(
                r'angular_distances\s*=\s*\(angular_distances\)\.cwiseMin\(symmetric_distances\)\s*;',
                'angular_distances = (angular_distances).cwiseMin(symmetric_distances).eval();',
                content)
            # Also handle case without parens wrapper
            content = re.sub(
                r'angular_distances\s*=\s*angular_distances\.cwiseMin\(symmetric_distances\)\s*;',
                'angular_distances = angular_distances.cwiseMin(symmetric_distances).eval();',
                content)
            # Nuclear option: replace 'auto angular_distances' with 'Eigen::ArrayXXf angular_distances'
            content = content.replace('auto angular_distances', 'Eigen::ArrayXXf angular_distances')
            content = content.replace('auto symmetric_distances', 'Eigen::ArrayXXf symmetric_distances')

            # Fix .pow({1}) -> .pow(1) - Eigen can't deduce template from initializer list
            content = content.replace('.pow({1})', '.pow(1)')

            with open('src/critics/goal_angle_critic.cpp', 'w') as f:
                f.write(content)
            PYEOF

            # Fix incomplete xtensor-to-Eigen migration in optimizer.cpp.
            # The Eigen optimization patch (a33e8d2b) failed to apply hunk #8 to this file,
            # so xt::clip calls and .shape() calls remain.
            # Also fix signature mismatch: patches left 'int' parameter where header expects 'const geometry_msgs::msg::Pose&'.
            python3 << 'PYEOF'
            import re
            with open('src/optimizer.cpp', 'r') as f:
                content = f.read()

            # Fix double comma from failed goal parameter removal
            content = content.replace('plan,, nav2_core', 'plan, nav2_core')
            content = content.replace('plan,, int', 'plan, int')

            # Fix .shape(0) -> .size() for control sequence
            content = content.replace('.shape(0)', '.size()')
            content = content.replace('.shape(1)', '.cols()')

            # Replace xt::clip with utils::clamp (array clamp with element-wise semantics)
            # xt::clip(val, min, max) -> val = val.max(min).min(max) for Eigen arrays
            content = re.sub(
                r'control_sequence_\.(\w+) = xt::clip\(control_sequence_\.\1,\s*(-?[\w.]+),\s*([\w.]+)\)',
                r'control_sequence_.\1 = control_sequence_.\1.max(\2).min(\3)',
                content
            )

            with open('src/optimizer.cpp', 'w') as f:
                f.write(content)
            PYEOF

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
            find_package(nav2_util REQUIRED)
            find_package(nav_msgs REQUIRED)
            find_package(pluginlib REQUIRED)
            find_package(rclcpp REQUIRED)
            find_package(tf2 REQUIRED)
            find_package(tf2_geometry_msgs REQUIRED)
            find_package(tf2_ros REQUIRED)
            find_package(visualization_msgs REQUIRED)

            include_directories(
              include
              ''${EIGEN3_INCLUDE_DIR}
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
            target_compile_options(mppi_controller PUBLIC -O3 -Wno-error=null-dereference)
            target_include_directories(mppi_controller
              PUBLIC
                "$<BUILD_INTERFACE:''${CMAKE_CURRENT_SOURCE_DIR}/include>"
                "$<INSTALL_INTERFACE:include/''${PROJECT_NAME}>")

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
            target_compile_options(mppi_critics PUBLIC -O3)
            target_include_directories(mppi_critics
              PUBLIC
                "$<BUILD_INTERFACE:''${CMAKE_CURRENT_SOURCE_DIR}/include>"
                "$<INSTALL_INTERFACE:include/''${PROJECT_NAME}>")
            target_link_libraries(mppi_critics PRIVATE
              pluginlib::pluginlib
            )

            set(libraries mppi_controller mppi_critics)

            foreach(lib IN LISTS libraries)
              ament_target_dependencies(''${lib} PUBLIC
                angles
                Eigen3
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
              )
            endforeach()

            install(TARGETS mppi_controller mppi_critics
              EXPORT nav2_mppi_controller
              ARCHIVE DESTINATION lib
              LIBRARY DESTINATION lib
              RUNTIME DESTINATION bin
            )

            install(DIRECTORY include/
              DESTINATION include/''${PROJECT_NAME}
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
            ament_export_include_directories(include/''${PROJECT_NAME})
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
