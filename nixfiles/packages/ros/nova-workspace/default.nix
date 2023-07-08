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
, nova-core ? throw "core is needed, but not available!"
, nova-control ? throw "control is needed, but not available!"
, nova-autonomous ? throw "autonomous is needed, but not available!"
, nova-electronics ? throw "electronics is needed, but not available!"
, nova-science ? throw "science is needed, but not available!"
, nova-cameras2 ? throw "cameras2 is needed, but not available!"
, nova-gui-backend ? throw "gui-backend is needed, but not available!"
, nova-gui-frontend ? throw "gui-frontend is needed, but not available!"
, nova-gui-frontend-server ? throw "gui-frontend-server is needed, but not available!"

  # Configuration options
  ## Include graphical applications in the workspace.
, graphical ? true

  ## Configure the workspace for interactive use.
, interactive ? true

  ## Manually specify which Nova Rover packages to include.
  ## Note that some packages may have dependencies on others that will be
  ## implicitly included.
, novaPackages ? [
    nova-core
    nova-control
    nova-autonomous
    nova-electronics
    nova-science
    nova-cameras2
    nova-gui-backend
    nova-gui-frontend
    nova-gui-frontend-server
  ]
}:

let
  extraPackages = [
    ros-core # https://github.com/ros2/variants
  ] ++ lib.optionals interactive ([
    (writeShellScriptBin "mk-nova-shell-setup"
      "cat ${substituteAll {
        name = "nova-shell-setup.sh";
        src = ./shell_setup.sh;
        inherit runtimeShell;
        argcomplete = python.pkgs.argcomplete;
      }}")
  ] ++ lib.optionals graphical [
    rviz2
  ]);

  splitRosPackages = builtins.partition (pkg: pkg.rosPackage or false);
  splitNovaPackages = splitRosPackages novaPackages;
  splitExtraPackages = splitRosPackages extraPackages;

  novaRosPackages = splitNovaPackages.right;
  novaOtherPackages = splitNovaPackages.wrong;
  extraRosPackages = splitExtraPackages.right;
  extraOtherPackages = splitExtraPackages.wrong;

  # The ROS overlay's buildEnv has special logic to wrap ROS packages so that
  # they can find each other.
  # Unlike the regular buildEnv from Nixpkgs, however, it is designed only with
  # nix-shell in mind, and only propagates non-ROS packages rather than
  # including them.
  # We must use a combination of the ROS buildEnv and Nixpkgs buildEnv to
  # include all packages in the environment.
  workspace = pkgs.buildEnv {
    name = "nova-workspace";
    paths =
      [ (buildEnv { paths = novaRosPackages ++ extraRosPackages; }) ]
      ++ novaOtherPackages ++ extraOtherPackages;
    passthru = {
      inherit
        novaPackages extraPackages
        novaRosPackages extraRosPackages
        novaOtherPackages extraOtherPackages;
      env = workspaceEnv;
    };
  };

  # A development environment for all Nova packages.
  workspaceEnv = mkShell {
    # Add non-Nova software to the environment.
    packages = extraOtherPackages ++
      [
        # Add colcon, for building packages.
        # This is a build tool that wraps other build tools, so it is not needed
        # normally in any of the ROS derivations and must be manually added here.
        colcon

        # Build a ROS environment for non-Nova ROS packages.
        (buildEnv { paths = extraRosPackages; })
      ];

    # Add the build inputs from Nova packages to the environment, so that they
    # can be built manually during development.
    inputsFrom = novaPackages;

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
