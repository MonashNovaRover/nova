{
  # Nixpkgs functions
  buildEnv
, mkShell
, writeShellScriptBin

  # General packages
, python
, rmw-fastrtps-dynamic-cpp
, ros-core
, colcon
, rviz2

  # Nova Rover packages
, core
, control
, autonomous
, electronics
, science
, cameras2

, wrapPrograms ? true
}:

let
  novaPackages = [
    core
    control
    autonomous
    electronics
    science
    cameras2
  ];

  packages = [
    rmw-fastrtps-dynamic-cpp # https://github.com/lopsided98/nix-ros-overlay/issues/45
    ros-core # https://github.com/ros2/variants
    python.pkgs.argcomplete
    rviz2 # Note: Cannot run without Xwayland (https://github.com/ros-visualization/rviz/issues/1442)

    (writeShellScriptBin "mk-nova-shell-setup" "cat ${./setup.sh}")
  ];

  mkEnv = { wrapPrograms }: buildEnv {
    inherit wrapPrograms;

    paths = packages ++ novaPackages;

    postBuild = ''
      # https://github.com/lopsided98/nix-ros-overlay/issues/45
      rosWrapperArgs+=(--set-default RMW_IMPLEMENTATION rmw_fastrtps_dynamic_cpp)
    '';
  };
in
(mkEnv { inherit wrapPrograms; }).overrideAttrs ({ passthru ? { }, ... }: {
  passthru = passthru // {
    # The env shell made by the ROS buildEnv function does not include development dependencies of packages.
    # Here it is replaced to do so.
    #
    # This allows for an entire workspace to be managed manually with colcon, if multiple packages need working on.
    # To work on individual packages without managing the whole workspace, consider opening a development shell
    # for that package instead (e.g. `nix-shell -A ros.nova.core`).
    env = mkShell {
      # Add general packages to the environment.
      packages = packages ++ [
        # Add colcon, for building packages.
        # This is a build tool that wraps other build tools, so it is not needed
        # normally in any of the ROS derivations and must be manually added here.
        colcon
      ];

      # Use the build inputs from Nova packages, so that they can be built manually for development.
      inputsFrom = novaPackages;

      # TODO: It would be nice if the split between environment and development packages could be mannually adjusted.
      # For example, if someone does not want to pull down all the development dependencies for autonomous, they could
      # have it added to packages rather than inputsFrom.

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
  };
})
