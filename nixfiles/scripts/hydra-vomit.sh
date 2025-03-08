#! /usr/bin/env bash

# This script builds the given jobset and tries to copy everything to the hydra binary cache, given:
#   - The jobset as $1 (defaulting to "workspaces" when unspecified),
#   - The path to the nova-oracle.key as $2 (defaulting to "~/nova-oracle.key" when unspecified),
#
# , assuming you have the nova-oracle.key in ~, and the mono-repo cloned in ~

# Update git
echo "Refreshing git repo"
git -C ~/nova pull
git switch master

jobset=${1:-workspaces}
oraclekey=${2:-~/nova-oracle.key}

echo "Uploading jobset \"$jobset\""
echo "Using SSH key \"$oraclekey\""
echo ""

nix-instantiate ~/nova/nixfiles/ci/jobsets/"$jobset".nix \
  --arg nixpkgs '<nixpkgs>' \
  --arg nova-monorepo ~/nova \
  --arg supportedSystems '[ "x86_64-linux" ]' \
  --argstr rosDistro jazzy \
  -A x86_64-linux \
| NIX_SSHOPTS="-i $oraclekey" xargs nix-copy-closure --to root@hydra.novarover.space --use-substitutes --log-format bar-with-logs

nix-instantiate ~/nova/nixfiles/ci/jobsets/"$jobset".nix \
  --arg nixpkgs '<nixpkgs>' \
  --arg nova-monorepo ~/nova \
  --arg supportedSystems '[ "x86_64-linux" ]' \
  --argstr rosDistro jazzy \
  -A x86_64-linux \
| xargs nix-store --query --requisites \
| xargs nix-store --realise \
| NIX_SSHOPTS="-i $oraclekey" xargs nix-copy-closure --to root@hydra.novarover.space --use-substitutes --log-format bar-with-logs
