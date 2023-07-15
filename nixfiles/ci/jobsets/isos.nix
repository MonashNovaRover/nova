{ supportedSystems
, nixpkgs
, src
, ...
}@args:

let
  lib = import ../lib.nix args;

  mkIso = system:
    { graphical ? false, includeWorkspace ? false }:
    let
      nova = lib.novaFor system;

      inherit (import ../workspaces.nix { inherit lib nova; })
        workspace
        hydraPatchedWorkspace;

      baseSystem = (import (nixpkgs + /nixos/lib/eval-config.nix) {
        system = nova.pkgs.hostPlatform.system;
        modules = [
          nova.nixosModule
          ../../nixos/installer/cd-dvd/nova-installation-cd-${if graphical then "graphical" else "base"}.nix
          {
            isoImage = {
              isoBaseName = "nixos-nova${lib.releaseLib.pkgs.lib.optionalString graphical "-graphical"}${lib.releaseLib.pkgs.lib.optionalString includeWorkspace "-workspace"}";
            };

            nova = {
              desktop.enable = graphical;
              substituters.nova.password = "***REMOVED***";
            };
            home-manager.sharedModules = [{
              # The Home Manager manual causes issues on Hydra.
              manual = {
                html.enable = false;
                manpages.enable = false;
                json.enable = false;
              };
              nova = {
                workspace = {
                  enable = includeWorkspace;
                  package = hydraPatchedWorkspace;
                };
              };
            }];
          }
        ];
      });

      extensions =
        # Note: Ideally, we would use lib.optionals, nova.pkgs.hostPlatform and
        # lib.systems.elaborate here to avoid using hardcoded strings and
        # verbose syntax.
        #
        # For some bizzare reason, however, the assertion added in
        # https://github.com/NixOS/nixpkgs/pull/238154 fails *later on* whenever
        # lib (from either nova.pkgs or lib.releaseLib.pkgs) is used in exactly
        # this place - even if it's just in a trace statement with
        # builtins.trace. (nova.pkgs.hostPlatforms must use lib at some stage.)
        #
        # This is likely due to a bug in Nix or Nixpkgs (expression evaluations
        # are not supposed to have side effects!), and may have something to do
        # with the "poor man's memoization" in releaseLib.pkgsFor.
        #
        # TODO: Make a minimal reproduction and submit a bug report in the Nix
        # issue tracker.
        if system == "x86_64-linux" && builtins.currentSystem == "aarch64-linux" then [
          # Some build tools are prohibitively slow in QEMU.
          # We can use the host tools to build the ISO.
          (prev: [{
            system.build.squashfsStore = lib.releaseLib.pkgs.lib.mkForce (prev.system.build.squashfsStore.override {
              inherit (lib.releaseLib.pkgsFor builtins.currentSystem)
                squashfsTools;
            });
          }])
          (prev: [{
            system.build.isoImage = lib.releaseLib.pkgs.lib.mkForce (prev.system.build.isoImage.override {
              inherit (lib.releaseLib.pkgsFor builtins.currentSystem)
                xorriso
                zstd;
            });
          }])
        ] else [ ];

      config = (builtins.foldl'
        (system: mkModules: system.extendModules { modules = mkModules system.config; })
        baseSystem
        extensions
      ).config;
    in
    config.system.build.isoImage;

  isoJobs = lib.releaseLib.forAllSystems (system: {
    iso-base = mkIso system { };
    iso-base-workspace = mkIso system { includeWorkspace = true; };
    iso-graphical = mkIso system { graphical = true; };
    iso-graphical-workspace = mkIso system { graphical = true; includeWorkspace = true; };
  });
in
isoJobs
