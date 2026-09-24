# Pinned out of tree nix modules

Adding this folder to your nix path makes `<jetpack-nixos>` evaluate to our pinned revisions.

Add to your `/etc/nixos/configuration.nix`:

```nix
{ ... }:
{
  environment.extraInit = ''
    export NIX_PATH="/home/nova/nova/nixfiles/modules/pinned''${NIX_PATH:+:$NIX_PATH}"
  '';
}
```

This won't apply till you rebuild nixos, so to set it temporarily use `NIX_PATH=$NIX_PATH:/home/nova/nova/nixfiles/modules/pinned` the first time, for example:

```bash
NIX_PATH=$NIX_PATH:/home/nova/nova/nixfiles/modules/pinned sudo -E nixos-rebuild switch
```

`sudo` needs the `-E` for the change in NIX_PATH to impact the nixos-rebuild that runs as root.
