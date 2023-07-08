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

## The Nova Rover binary cache

[Our cache](https://app.cachix.org/cache/nova) contains prebuilt ROS packages
and other dependencies of our software. In the future, it can be used to store
our software itself.

Follow the [ROS binary cache](https://github.com/lopsided98/nix-ros-overlay#configure-binary-cache)
instructions, replacing `ros` with `nova` where appropriate.

The public key is `nova.cachix.org-1:lGXmFv5muzN5S4Q1CHFqgDC8c0ponqy5albHQMNE5C8=`.