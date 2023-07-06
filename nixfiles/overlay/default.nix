self: super:

super.lib.composeManyExtensions [
  (import ./lib.nix)
  (import ./maintanence.nix)
  (import ./ros)
]
  self
  super
