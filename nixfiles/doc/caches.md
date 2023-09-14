# Binary caches

Nix can fetch prebuilt copies of packages, avoiding building them from source,
if they are available in a binary cache.

The use of the following binary caches is recommended.

## Official Nix binary cache

Nix uses [this cache](https://cache.nixos.org) by default. It contains prebuilt
copies of all packages in [Nixpkgs](https://github.com/NixOS/nixpkgs), the Nix
package collection. Nixpkgs (and subsequently the cache) also contains the
implementation of NixOS.

## The ROS binary cache

The [ROS binary cache](https://github.com/lopsided98/nix-ros-overlay#configure-binary-cache)
is built automatically whenever the [ROS overlay for the Nix package manager](https://github.com/lopsided98/nix-ros-overlay)
is updated. Unfortunately, this is not a frequent occurance, and we often need
to make modifications to the overlay. Invalidated packages will be made
available through our cache instead.

### Setup

When using the [NixOS module](./nixos.md), set [`nova.substituters.ros.enable`](https://hydra.novarover.space/manual/nixos#novasubstitutersrosenable) to
`true`. Otherwise, follow the instructions on the project page.

## The Nova Rover binary cache

[Our cache](https://hydra.novarover.space/jobset/nova/workspaces#tabs-jobs)
contains prebuilt copies of our ROS packages and their build and runtime
dependencies.

### Setup

When using the [NixOS module](./nixos.md), configure the [`nova.substituters.nova`](https://hydra.novarover.space/manual/nixos#novasubstitutersnovaenable)
options. Otherwise, add the [Hydra server](./hydra.md) to the [substituter list](https://nixos.org/manual/nix/unstable/command-ref/conf-file.html#conf-substituters),
along with a [netrc file](https://nixos.org/manual/nix/unstable/command-ref/conf-file.html#conf-netrc-file) including the required credentials.

The public key is `***REMOVED***`.

## The CUDA binary cache

The [cuda-maintainers binary cache](https://github.com/SomeoneSerge/nixpkgs-cuda-ci)
contains prebuilt copies of CUDA-enabled packages from Nixpkgs.

### Setup

To use it, add `https://cuda-maintainers.cachix.org` to the [substituter list](https://nixos.org/manual/nix/unstable/command-ref/conf-file.html#conf-substituters),
and add the public key to the [trusted list](https://nixos.org/manual/nix/unstable/command-ref/conf-file.html#conf-trusted-public-keys).

On NixOS, the `nix.settings` options can be used to do this.

The public key is `cuda-maintainers.cachix.org-1:0dq3bujKpuEPMCX6U4WylrUDZ9JyUG0VpVZa7CNfq5E=`.