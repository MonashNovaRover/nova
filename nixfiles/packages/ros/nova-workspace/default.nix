{
  # Nixpkgs functions
  lib
, substituteAll
, writeShellScriptBin
, buildEnv
, mkShell

  # General packages
, pkgs # Used ONLY to access attributes shadowed by the ROS overlay
, runtimeShell
, python
, rmw-fastrtps-dynamic-cpp
, ros-core
, colcon
, rviz2

  # Nova Rover packages
, nova-core
, nova-control
, nova-autonomous
, nova-electronics
, nova-science
, nova-cameras2
, nova-gui-backend
, nova-gui-frontend
, nova-gui-frontend-server

  # Configuration options
, includeGraphicalApplications ? true
}:

let
  novaRosPackages = [
    nova-core
    nova-control
    nova-autonomous
    nova-electronics
    nova-science
    nova-cameras2
    nova-gui-backend
  ];

  novaOtherPackages = [
    nova-gui-frontend
    nova-gui-frontend-server
  ];

  rosPackages = [
    ros-core # https://github.com/ros2/variants
  ] ++ lib.optionals includeGraphicalApplications [
    rviz2 # Note: Cannot run without Xwayland (https://github.com/ros-visualization/rviz/issues/1442)
  ];

  otherPackages = [
    # Nix-related utility scripts
    (writeShellScriptBin "mk-nova-shell-setup"
      "cat ${substituteAll {
        name = "nova-shell-setup.sh";
        src = ./shell_setup.sh;
        inherit runtimeShell;
        argcomplete = python.pkgs.argcomplete;
      }}")
  ];

  # The ROS overlay's buildEnv has special logic to wrap ROS packages so that
  # they can find each other.
  # Unlike the regular buildEnv from Nixpkgs, however, it is designed only with
  # nix-shell in mind, and only propagates non-ROS packages rather than
  # including them.
  # We must use a combination of the ROS buildEnv and Nixpkgs buildEnv to
  # include all packages in the environment.
  rosEnv = buildEnv {
    paths = novaRosPackages ++ rosPackages ++ [
      # https://github.com/lopsided98/nix-ros-overlay/issues/45
      rmw-fastrtps-dynamic-cpp
    ];

    postBuild = ''
      # https://github.com/lopsided98/nix-ros-overlay/issues/45
      rosWrapperArgs+=(--set-default RMW_IMPLEMENTATION rmw_fastrtps_dynamic_cpp)
    '';
  };

  workspace = pkgs.buildEnv {
    name = "nova-workspace";
    paths = [ rosEnv ] ++ novaOtherPackages ++ otherPackages;
    passthru.env = workspaceEnv;
  };

  # A development environment for all Nova packages.
  workspaceEnv = mkShell {
    # Add non-Nova software to the environment.
    packages = rosPackages ++ otherPackages ++ [
      # https://github.com/lopsided98/nix-ros-overlay/issues/45
      rmw-fastrtps-dynamic-cpp

      # Add colcon, for building packages.
      # This is a build tool that wraps other build tools, so it is not needed
      # normally in any of the ROS derivations and must be manually added here.
      colcon
    ];

    # Add the build inputs from Nova packages to the environment, so that they
    # can be built manually during development.
    inputsFrom = novaRosPackages ++ novaOtherPackages;

    shellHook = ''
      # https://github.com/lopsided98/nix-ros-overlay/issues/45
      export RMW_IMPLEMENTATION=rmw_fastrtps_dynamic_cpp

      if [ -z "$NIX_EXECUTING_SHELL" ]; then
        eval "$(mk-nova-shell-setup)"
      else
        # If a different shell is in use through a tool like https://github.com/chisui/zsh-nix-shell,
        # this hook will not be running in it. "mk-nova-shell-setup" must be run manually.
        if [ -z "$I_WILL_RUN_NOVA_SHELL_SETUP" ]; then
          echo >&2 'The shell setup script must be manually run.'
          echo >&2 '$ eval "$(mk-nova-shell-setup)"'
          echo >&2 'Set I_WILL_RUN_NOVA_SHELL_SETUP=1 to silence this message.'
        fi
      fi
    '';
  };
in
workspace
