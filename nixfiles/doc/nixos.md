# Installing NixOS

## Bare metal

ISO images of NixOS with our custom modules are available from [Hydra](./hydra.md).

The following links go to the latest generic PC ISO builds. There are four variants:

- [base](https://hydra.novarover.space/job/nova/isos/x86_64-linux.iso-base/latest/download-by-type/file/iso): The standard NixOS installer, with the `nova`
user and binary cache
- [base-workspace](https://hydra.novarover.space/job/nova/isos/x86_64-linux.iso-base-workspace/latest/download-by-type/file/iso): `base` + our software preinstalled
- [graphical](https://hydra.novarover.space/job/nova/isos/x86_64-linux.iso-graphical/latest/download-by-type/file/iso): `base` + the standard team desktop environment
- [`graphical-workspace`](https://hydra.novarover.space/job/nova/isos/x86_64-linux.iso-graphical-workspace/latest/download-by-type/file/iso): `base-workspace` + `graphical`

Builds for other types of devices, including generic AArch64 devices, NVIDIA Jetson SoMs, and MacBooks with Apple T2 chips, are also available. See the [complete Hydra jobset](https://hydra.novarover.space/jobset/nova/isos/latest-eval).

The `workspace` variants are recommended for trying out all of the features
before installing. As well as including all our software preinstalled, they ship
with source code in `~/nova/src` and build dependencies in the Nix store. You can
boot the ISO and work on any package of ours immediately.

The non-workspace variants are recommened for pure installation purposes.
[The GNOME Partition Editor (GParted)](https://gparted.org) is included in all `graphical` variants.

Follow the NixOS [manual installation instructions](https://nixos.org/manual/nixos/unstable/index.html#sec-installation-manual).
When the time comes to edit `configuration.nix`, follow the instructions below
to add the NixOS module. It is important that this is done at this stage, and
not after installation, as the ISO is configured with our binary caches and the
installed system will not be.

If the `shared` profile is not in use, you will need to create a user account
for yourself with [`nova.users`](https://hydra.novarover.space/manual/nixos#novausersnameenable).
Choose `nova` as the usename to add the standard team user, or choos something
else if you do not want this.

## Virtualisation

NixOS can be virtualised in a container or virtual machine.
See: [NixOS virtualisation](./virtualisation.md)

# Using the NixOS module

Include the [NixOS](https://nixos.org/manual/nixos/stable) module with an import, like so:

```nix
{
  imports = [
    ./path/to/repo/modules/nixos
  ];
}
```

You should set state versions and a _Nova device profile_.

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

  nova.substituters.nova.password = builtins.readFile ./path/to/hydra-password.txt;
}
```

Available options can be found in the [option documentation](https://hydra.novarover.space/manual/nixos).