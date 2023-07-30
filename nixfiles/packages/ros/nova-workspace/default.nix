{ lib
, buildROSWorkspace
, rviz2

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

  ## Extra packages to add to the workspace.
, extraPackages ? [ ]
}:

(buildROSWorkspace {
  name = "nova";
  inherit interactive;
  devPackages = novaPackages;
  prebuiltPackages = lib.optionals graphical [
    rviz2
  ]
  ++ extraPackages;
}).overrideAttrs ({ passthru ? { }, ... }: {
  passthru = passthru // {
    inherit novaPackages extraPackages;
  };
})
