# Nixfiles

This repository contains [Nix](https://nixos.org) files for:
- Setting up a ROS workspace and build environment for our software (with plain Nix, for any Linux distro)
- Configuring user environments (with [Home Manager](https://github.com/nix-community/home-manager), for any Linux distro)
- Configuring NixOS for various devices
- Setting up [Hydra](https://nixos.org/hydra) for CI/CD

## Usage

### ROS workspace and software

See: [Using the Nix packages](./doc/nix.md)

### User environments

See: [Using the Home Manager module](./doc/home-manager.md)

### NixOS

See: [Using the NixOS module](./doc/nixos.md)

### Hydra

See: [Hydra](./doc/hydra.md)