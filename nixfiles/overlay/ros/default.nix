self: super:

super.lib.composeManyExtensions [
  (import ./util.nix)
  (import ./version.nix)
  (import ./lib.nix)
  (import ./qol.nix)
  (import ./maintanence.nix)
  (import ./backports.nix)
]
  self
  super
