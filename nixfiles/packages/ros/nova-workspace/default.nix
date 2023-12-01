{ lib
, buildROSWorkspace
, buildEnv
, rviz2
, gazebo
, rqt
, rqt-common-plugins
, gdb

, nova-core ? throw "core is needed, but not available!"
, nova-control ? throw "control is needed, but not available!"
, nova-autonomous ? throw "autonomous is needed, but not available!"
, nova-electronics ? throw "electronics is needed, but not available!"
, nova-science ? throw "science is needed, but not available!"
, nova-cameras2 ? throw "cameras2 is needed, but not available!"
, nova-gui-backend ? throw "gui-backend is needed, but not available!"
, nova-gui-frontend ? throw "gui-frontend is needed, but not available!"
, nova-gui-frontend-server ? throw "gui-frontend-server is needed, but not available!"
, nova-blcmd-hardware ? throw "nova-rover_hardware is needed, but not available!"
#, nova-four-wheel-steering-controller ? throw "nova-four-wheel-steering-controller is needed, but not available!"
#, nova-four-steering-controller ? throw "nova-four-steering-controller is needed, but not available!"
, nova-pivot-drive-controller ? throw "nova-pivot-drive-controller is needed, but not available!"

  # Configuration options
  ## Include graphical applications in the workspace.
, graphical ? true

  ## Configure the workspace for interactive use.
, interactive ? true

  ## Manually specify which Nova Rover packages to include.
  ## Note that some packages may have dependencies on others that will be
  ## implicitly included.
, novaPackages ? {
    inherit
      nova-core
      nova-control
      nova-autonomous
      nova-electronics
      nova-science
      nova-cameras2
      nova-gui-backend
      nova-gui-frontend
      nova-blcmd-hardware
      #nova-four-wheel-steering-controller
      #nova-four-steering-controller;
      nova-pivot-drive-controller;
  }

  ## Extra packages to add to the workspace.
, extraPackages ? {
    inherit
      # Some of our packages are simple constructions written in Nix and do not
      # need to be considered in the build environment.
      nova-gui-frontend-server;
  }
}:

let
  buildWorkspace = buildROSWorkspace.override {
    buildROSWorkspace = buildWorkspace;
    buildROSEnv = args: buildEnv (args // {
      # There are too many packages to completely avoid collisions.
      # Warnings during build time should be carefully observed.
      ignoreCollisions = true;
    });
  };
in
(buildWorkspace {
  name = "nova";
  inherit interactive;
  devPackages = novaPackages;
  prebuiltPackages = (lib.optionalAttrs graphical {
    inherit
      rviz2
      gazebo
      rqt rqt-common-plugins;
  }) // extraPackages;
  prebuiltShellPackages = {
    inherit
      gdb;
  };
}).overrideAttrs ({ passthru ? { }, ... }: {
  passthru = passthru // {
    inherit novaPackages extraPackages;
  };
})
