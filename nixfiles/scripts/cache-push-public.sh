#! /usr/bin/env nix-shell
#! nix-shell -i bash -p cachix

# This script builds an empty Nova workspace and development environment, and
# pushes the closures to the public Cachix cache.
# As the workspace is empty, no Nova software is made publicly available.

set -e

nix-build --log-format bar-with-logs --no-out-link -E \
  'with (import ./. { repos = [ ]; }).pkgs; let
    workspace = ros.nova-workspace.override { novaPackages = [ ]; };
  in
  [
    workspace
    workspace.env.inputDerivation
  ]' | cachix push nova