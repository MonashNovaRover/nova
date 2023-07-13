{ lib, nova }:

rec {
  workspace = nova.pkgs.ros.nova-workspace;
  hydraPatchedWorkspace = workspace.override
    # Some x86_64 packages fail to build in QEMU on Aarch64. Workarounds
    # must be made to avoid these failures.
    # There is no easy way at this stage to determine if Hydra has access to
    # any real x86_64 machines, so these changes will apply indiscriminately.
    (lib.releaseLib.pkgs.lib.optionalAttrs (nova.pkgs.hostPlatform.isx86_64 && (lib.releaseLib.pkgs.lib.systems.elaborate builtins.currentSystem).isAarch64) {
      novaPackages =
        let
          # The GUI frontend fails to build, but the output contains only
          # static Web assets, and is architecture-independent. Use the
          # Aarch64 version instead.
          nova-gui-frontend = (lib.novaFor "aarch64-linux").pkgs.nova-gui-frontend;
          nova-gui-frontend-server = nova.pkgs.nova-gui-frontend-server.override { inherit nova-gui-frontend; };

          added = [
            nova-gui-frontend
            nova-gui-frontend-server
          ];
          removed = [
            nova.pkgs.nova-gui-frontend
            nova.pkgs.nova-gui-frontend-server
          ];

          removedNames = map lib.releaseLib.pkgs.lib.getName removed;
        in
        (builtins.filter (pkg: !builtins.elem (lib.releaseLib.pkgs.lib.getName pkg) removedNames) workspace.novaPackages) ++ added;
    });
}
