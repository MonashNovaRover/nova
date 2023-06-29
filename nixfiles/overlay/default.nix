self: super:

super.lib.composeManyExtensions [
  (import ./lib.nix)
  (import ./ros)
  (import ./maintanence.nix)
]
  self
  super
