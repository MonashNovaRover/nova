self: super:

super.lib.composeManyExtensions [
  (import ./lib.nix)
  (import ./maintanence.nix)
  (import ./ros)
  (import ./customisations.nix)
]
  self
  super
