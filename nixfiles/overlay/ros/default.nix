self: super:

super.lib.composeManyExtensions [
  (import ./util.nix)
  (import ./version.nix)
  (import ./lib.nix)
  (import ./qol.nix)
  (import ./prerelease.nix)
  (import ./backports.nix)
  (import ./maintanence.nix)
  (import ./livox-ros-driver2.nix)
]
  self
  super
