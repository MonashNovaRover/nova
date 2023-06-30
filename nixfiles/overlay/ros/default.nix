self: super:

super.lib.composeManyExtensions [
  (import ./util.nix)
  (import ./version.nix)
  (import ./lib.nix)
  (import ./maintanence.nix)
]
  self
  super
