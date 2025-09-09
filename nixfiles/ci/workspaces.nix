{ lib, novaPkgs, rosDistro ? null }:

rec {
  workspace = (if rosDistro == null then novaPkgs.ros else novaPkgs.rosPackages.${rosDistro}).nova-workspace;
  hydraPatchedWorkspace = workspace.override
    # Some x86_64 packages fail to build in QEMU on Aarch64. Workarounds
    # must be made to avoid these failures.
    # There is no easy way at this stage to determine if Hydra has access to
    # any real x86_64 machines, so these changes will apply indiscriminately.
    (lib.pkgs.lib.optionalAttrs (novaPkgs.hostPlatform.isx86_64 && (lib.pkgs.lib.systems.elaborate builtins.currentSystem).isAarch64) rec {
      novaPackages = workspace.novaPackages;
      extraPackages = workspace.extraPackages;
    });
}
