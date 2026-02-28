# Nixfiles

This repository contains [Nix](https://nixos.org) files for:
- Setting up a ROS workspace and build environment for our software (with plain Nix, for any Linux distro)
- Configuring user environments (with [Home Manager](https://github.com/nix-community/home-manager), for any Linux distro)
- Configuring NixOS for various devices
- Setting up [Hydra](https://nixos.org/hydra) for CI/CD

## Project Structure

```
├ ci                     # Hydra configuration
├ doc                    # Documentation for our nixos/package systems
├ external               # Templates for how nova's and out-of-tree packages should be defined
├ lib                    # Library of nix functions to help with nova packaging
├ modules                # All toggleable nova modules live here
| ├ common               # Legacy from before monorepo
| ├ home                 # Home manager only options, see: https://hydra.novarover.space/manual/home-manager or https://hydra.novarover.space/manual/nixos or https://home-manager-options.extranix.com/
| | ├ ...                
| | └ macros             # Scripts and aliases
| |   ├ launch           # Terminal launch scripts
| |   └ default.nix      # Alias definitions
| └ nixos                # Nixos stuff that you can enable through https://search.nixos.org/options?channel=unstable
├ overlay                # Modifying the default build path/ configuration for ANY nix package
├ packages               # Packing any out-of-nix packages into nix, normally from github
├ scripts                # Bash scripts
├ secrets                # Symlink to secrets repo
└ tests                  # End to end tests for the rover and base station using vms
```

## Contributing

### Code standards

Before modifying any Nix code, be sure to follow the [IDE setup guide](./doc/ides.md#nix).

### Adding a new repository

To use a new Nova software repository:

1. Ensure that the repository has a `default.nix` as described by [_Adding out-of-tree packages_](./doc/nix.md#adding-out-of-tree-packages).
2. Add the repository to the [checkout script](./scripts/checkout-nova-sources.sh).
3. Add the repository to [`nova-repos.nix`](./ci/nova-repos.nix).
4. Add the PR JSON input to [`spec.json`](./ci/spec.json).

## Usage

### ROS workspace and software

See: [Using the Nix packages](./doc/nix.md)

### User environments

See: [Using the Home Manager module](./doc/home-manager.md)

### NixOS

See: [Using the NixOS module](./doc/nixos.md)

### Hydra

See: [Hydra](./doc/hydra.md)
