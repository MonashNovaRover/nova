# Using the NixOS module

Include the [NixOS](https://nixos.org/manual/nixos/stable) module with an import, like so:

```nix
{
  imports = [
    (import ./path/to/repo { }).nixosModule
  ];
}
```

You must set state versions and a _Nova device profile_ for the configuration to work. For example:

```nix
{
  system.stateVersion = "23.05";
  home-manager.sharedModules = [{
    home.stateVersion = "23.05";
  }];

  nova.profile = "personal";
  # "personal" aims to be flexible for use on regular NixOS installations.
  # Use "shared" to enable all the things on a team device - branding, the
  # standard user and desktop environment, etc.
  # These things can all be enabled manually even on "personal" configurations,
  # e.g. with nova.desktop.enable = true.

  nova.substituters.nova.password = "***REMOVED***";
}
```

Available options can be found [in the module definitions](../nixos).