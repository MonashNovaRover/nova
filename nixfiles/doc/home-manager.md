# Using the Home Manager module

[Home Manager](https://github.com/nix-community/home-manager) allows user environments (including "dotfiles" and installed packages) to be configured with Nix.

This repository includes a custom Home Manager module for team configurations.

> The [NixOS module](../doc/nixos.md) includes the Home Manager module, so the following steps do not need to be done when using it.

Add it to your Home Manager configuration with an import, like so:

```nix
{
  imports = [
    ./path/to/repo/modules/home
  ];
}
```

Users called `nova` will have a standard team configuration enabled. If you do
not want this, choose another username.

Available options can be found in the [option documentation](https://hydra.novarover.space/manual/home-manager).