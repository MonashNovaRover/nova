self: super:

super.lib.composeManyExtensions [
  (import ./version.nix)
  (import ./lib.nix)
  (import ./maintanence.nix)
]
  self
  super
