#! /usr/bin/env bash

# This workflow automatically updates our nixpkgs and nix-ros-overlay versions, using an LLM to fix ws-build errors.
# See https://github.com/MonashNovaRover/nova/issues/1183 for more.

## prechecks:
# Confirm in ~/nova
cd $HOME/nova

# Configure git
export GIT_AUTHOR_NAME=github-actions[bot]
export GIT_AUTHOR_EMAIL=github-actions[bot]@users.noreply.github.com
export GIT_COMMITTER_NAME=github-actions[bot]
export GIT_COMMITTER_EMAIL=github-actions[bot]@users.noreply.github.com
export GIT_BRANCH=test/update-nix2

# Checkout the branch if it already exists
git pull
if git rev-parse --verify --quiet origin/$GIT_BRANCH >/dev/null; then
    git checkout $GIT_BRANCH
else
    # Create the branch if it does not exist
    git checkout -b $GIT_BRANCH && git push --set-upstream origin $GIT_BRANCH
fi
git pull

## update-nixpkgs:
# Outputs the latest pinned nixpkgs version used by nix-ros-overlay.
nix-env -f '<nixpkgs>' -iA jq
curl -L https://raw.githubusercontent.com/lopsided98/nix-ros-overlay/refs/heads/develop/flake.lock -o flake.lock
export NIXPKGS_SHA=$(jq -r '.nodes.nixpkgs.locked.rev' flake.lock)
export NIXPKGS_HASH=$(jq -r '.nodes.nixpkgs.locked.narHash' flake.lock)

## update-nix-ros-overlay:
# Outputs the latest nix-ros-overlay version.
nix-env -f '<nixpkgs>' -iA nurl
export NIX_ROS_OVERLAY_SHA=$(git ls-remote https://github.com/lopsided98/nix-ros-overlay develop | awk -F'\t' '{print $1}')
export NIX_ROS_OVERLAY_HASH=$(nurl https://github.com/lopsided98/nix-ros-overlay $NIX_ROS_OVERLAY_SHA --hash)

## update-revisions:
# Pushes the latest versions of nixpkgs and nix-ros-overlay to a new branch.

# Write the latest versions of nixpkgs and nix-ros-overlay
jq '.nixpkgs.rev=$ENV.NIXPKGS_SHA' nixfiles/revisions.json > tmp.json && mv tmp.json nixfiles/revisions.json
jq '.nixpkgs.hash=$ENV.NIXPKGS_HASH' nixfiles/revisions.json > tmp.json && mv tmp.json nixfiles/revisions.json
jq '."nix-ros-overlay".rev=$ENV.NIX_ROS_OVERLAY_SHA' nixfiles/revisions.json > tmp.json && mv tmp.json nixfiles/revisions.json
jq '."nix-ros-overlay".hash=$ENV.NIX_ROS_OVERLAY_HASH' nixfiles/revisions.json > tmp.json && mv tmp.json nixfiles/revisions.json

# Push any changes
if ! git diff --quiet; then
    git add .
    git commit -a -m "setup: Updating nixpkgs and nix-ros-overlay to the latest versions"
    git push
fi

## fix-errors:
# Checks out the new branch and uses an LLM to fix ws-build errors, pushing fixes.

# Install Nix packages to run LLM
nix-env -f '<nixpkgs>' -iA mcp-nixos
nix-env -f '<nixpkgs>' -iA opencode

# Remove nixfiles/secrets
git rm -f nixfiles/secrets

# Push any changes
if ! git diff --quiet; then
    git add .
    git commit -a -m "setup: Hiding secrets from Sisyphus, ADD BACK BEFORE MERGING"
    git push
fi

# Make sure remaining submodules are checked out
git submodule update --init --recursive

# To the same above for each cloned submodule
git submodule foreach --recursive '
    # Checkout the branch if it already exists
    git pull origin master
    if git rev-parse --verify --quiet origin/$GIT_BRANCH >/dev/null; then
        git checkout $GIT_BRANCH
    else
        # Create the branch if it does not exist
        git checkout -b $GIT_BRANCH && git push --set-upstream origin $GIT_BRANCH
    fi
    git pull
'

# # Run LLM
# opencode run --model opencode/mimo-v2.5-free --agent patch --auto --print-logs --log-level DEBUG "/fix-errors"