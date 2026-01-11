# NixOS virtualisation

NixOS systems do not have to activate on bare metal; they can just as easily
activate in containers and virtual machines.

This repository contains prebuilt configurations for use in both containers and
virtual machines.

> While both these technologies can be used on any distribution with Nix,
> distributions other than NixOS may require [NixGL](https://github.com/guibou/nixGL)
> to prevent issues running graphical applications.

## Advantages and disadvantages of containers and VMs

Containers and virtual machines each have their own advantages and
disadvantages, particularly in the context of NixOS.

| Feature | Docker container | NixOS container | Nixos virtual machine | Regular virtual machine |
| :------ | :--------------: | :-------------: | :-------------------: | :---------------------: |
| Supported host platforms | All | Linux | Linux | All
| Startup time | Fast | Fast | Slow | Slow |
| Resource usage | Low | Low | High | High |
| Isolation | Medium | Low (see the [warning][nixos-container]) | High | High |
| System integration | Medium | High | Low | Medium |
| Nix store access | Duplicated | Shared with host | Shared or duplicated | Duplicated |
| Re-activation from host (like `nixos-rebuild switch`) | N/A | Fast | Slow | N/A |
| Emulated kernel | On non-Linux platforms | No | Yes | Yes |
| Accurate boot process | No (skips bootloader and NixOS stage 1) | No (skips bootloader and NixOS stage 1) | Partial | Yes |
| Hardware-accelerated graphics | N/A | No | [With effort][GVT-g] | Partial |

## Using containers

### NixOS containers

#### Creating NixOS containers

There are three NixOS container management interfaces, all based on
[systemd-nspawn](https://www.freedesktop.org/software/systemd/man/systemd-nspawn.html).

- [nixos-container] (declarative): NixOS's built-in container support.
  Containers are defined in and built with the host's system configuration.
- [nixos-container] (imperative): NixOS's imperative container management
  interface. Containers are created manually through the `nixos-container`
  command. Many features available in declarative containers are unsupported in
  imperative containers.
- [extra-container]: An implementation NixOS's declarative containers with an
  imperative CLI. "The best of both worlds". Avaliable on any distro with Nix
  installed.

Due to its flexibility, [extra-container] is recommended, though the following
instructions should be similar for the other container management interfaces as
well.

Note that graphical applications will not work in NixOS's imperative containers, due to the
[lack of bind-mounting](https://github.com/NixOS/nixpkgs/issues/18355).

1. [Install extra-container](https://github.com/erikarvstedt/extra-container#install)
2. Create a container
   ```
   $ extra-container create -E 'import ./modules/nixos/installer/container { }'
   ```
3. Start the container
   ```
   $ extra-container start nova
   ```
4. Log in to the container
   ```
   $ extra-container login nova
   ```
5. Start the desktop environment in a window (optional)
   ```
   # On the host system, allow remote X11 access
   $ xhost +local:
   ```
   ```
   # On the container
   $ startx-nested
   ```

#### Updating NixOS containers

Containers can be updated with a new system configuration at runtime, through
the same mechanism used by `nixos-rebuild switch`. The container configuration
can also be modified after creation (though this will often require the
container to be restarted to apply).

To do update or modify containers, run the creation command again with the same
name. Use the `--update-changed` (`-u`) flag to active the new system
configuration, and/or the `--restart-changed` (`-r`) flag to restart the
container.

#### Customising NixOS containers

`./modules/nixos/installer/container` can be called with the following arguments:

- `name`: The name of the container (`nova` by default)
- `modules`: A list of extra NixOS modules to add to the container (empty by default)

For example:

```
$ extra-container create -E 'import ./modules/nixos/installer/container {
    name = "nova-headless";
    modules = [{
      services.xserver.enable = false;
      nova.desktop.enable = false;
    }];
  }'
```

[nixos-container]: https://nixos.org/manual/nixos/unstable/index.html#ch-containers
[extra-container]: https://github.com/erikarvstedt/extra-container

### Docker containers

Docker images with our NixOS distribution can be downloaded from [Hydra](./hydra.md).

#### Creating Docker containers

1. Download an image for your platform:
   - [x86_64](https://hydra.novarover.space/job/nova/docker/x86_64-linux.base/latest/download-by-type/file/docker-image)
   - [AArch64](https://hydra.novarover.space/job/nova/docker/aarch64-linux.base/latest/download-by-type/file/docker-image)
1. Load the image:
   ```console
   $ docker load -i path/to/image.tar.gz
   ```
1. Create a container:
   > Note: it is highly recommended to [add a volume](https://docs.docker.com/storage/volumes)
   > for source code in this step. When following later setup instructions, clone
   > repositories into this volume and link them to their regular locations with `ln -s`.
   >
   > If you do not do this, you will be unable to easily access files from your
   > host OS.
   ```console
   $ docker create --privileged --network=host --hostname=nova-container --name=nova nova-nixos
   ```
1. Start the container:
   ```console
   $ docker start nova
   ```
1. Log in:
   ```console
   $ docker exec -it nova /run/current-system/sw/bin/login -f nova
   ```

#### Updating Docker containers

The Docker images do not ship with a `configuration.nix` - you'll need to make
one after installation.

1. Log in to GitHub:
   ```console
   $ gh auth login
   ```
1. Clone this repository:
   ```console
   $ gh repo clone MonashNovaRover/nixfiles ~/nova/nixfiles
   ```
1. Use the following template in `/etc/nixos/configuration.nix`:
   ```nix
   {
     imports = [
       /home/nova/nova/nixfiles/modules/nixos
       /home/nova/nova/nixfiles/modules/nixos/installer/docker
     ];

     nova.substituters.nova.password = builtins.readFile ./path/to/hydra-password.txt;
   }
   ```
1. Add the Home Manager channel:
   ```console
   $ sudo nix-channel --add https://github.com/nix-community/home-manager/archive/master.tar.gz home-manager
   $ sudo nix-channel --update
   ```

You can then use NixOS like normal.

## Using virtual machines

NixOS's first-party virtual machine derivation is fully supported, with useful
defaults.

### Creating virtual machines

To create a VM, simply build the derivation and run the result:

```
$ nix-build '<nixpkgs/nixos>' \
    --arg configuration ./modules/nixos \
    -A vm
```

If no disk image yet exists, one will be automatically created. This is a fast
process, as the Nix store is shared with the host by default and most of the
system files are set up at boot during the system activation stage.

### Updating virtual machines

The derivation can be rebuilt once the system configuration is modified by
running the build command again. The VM will need to be restarted for changes
to take effect.

### Customising virtual machines

The virtual machine system configuration can be changed by adding NixOS module
options under the [`virtualisation.vmVariant`](https://search.nixos.org/options?show=virtualisation.vmVariant)
prefix.

The `virtualisation.vmVariant` module also adds several options to configure the
virtual machine settings. These options are not visible in the NixOS manual,
but can be found in the `options` section of the [module source code](https://github.com/NixOS/nixpkgs/blob/nixos-unstable/nixos/modules/virtualisation/qemu-vm.nix).

A notable option is `useBootLoader`, which will use a more traditional boot
process and generate a disk image for the Nix store rather than mounting the
host's read-only. The persistent disk image is not compatible between VMs with
this option enabled and disabled, and must be changed in-between runs.

#### Hardware configuration

Customising the virtual machine to suit the hardware of the host platform is
recommended. A useful pattern is to put the VM hardware configuration in a
separate Nix file that can be used with all VMs. For example, if the file is put
in `~/nova/Documents/VM.nix`, the VM should be built like so:

```
$ nix-build '<nixpkgs/nixos>' \
    --arg configuration "{
      imports = [
        ./modules/nixos
        ~/nova/Documents/VM.nix
      ];
    }" \
    -A vm
```

##### Example hardware configuation

An example configuration with custom system specifications,
[SPICE](https://www.spice-space.org), a copy of [QEMU](https://www.qemu.org)
patched for 60Hz, and GPU access through [GVT-g] is given below.

Note that GVT-g may cause X11 to hang briefly during startup, and may [require
a larger memlock limit on the host system](https://github.com/intel/gvt-linux/issues/69#issue-406134079)
or even a [jailbroken UEFI firmware setting](https://github.com/intel/gvt-linux/issues/131).

<details>
  <summary>Click to expand</summary>

  ```nix
  {
    virtualisation.vmVariant = ({ options, config, pkgs, ... }: {
      services.xserver.videoDrivers = [ "modesetting" ];
      services.spice-vdagentd.enable = true;
      services.spice-webdavd.enable = true;
      virtualisation = {
        host.pkgs = import <nixpkgs> { };
        useEFIBoot = true;
        cores = 2;
        memorySize = 4096;
        resolution = { x = 1280; y = 720; };
        qemu = {
          package = options.virtualisation.qemu.package.default.overrideAttrs ({ patches ? [ ], ... }: {
            # Warning: These modifications will cause QEMU to be built locally
            # rather than being pulled down from the NixOS binary cache.
            patches = patches ++ [
              (pkgs.fetchpatch {
                url = "https://github.com/qemu/qemu/commit/67036ce24594cd04690e83a4cb005672cf2a9668.patch";
                hash = "sha256-aJD8U5u8v9wVab1WzHLI1yPm6kiPxbsI8mOkY1RX7UU=";
              })
            ];
          });
          options = [
            "-vga none"
            "-display spice-app,gl=on"
            "-spice image-compression=off,playback-compression=off,streaming-video=off"
            "-device ${builtins.concatStringsSep "," [
              "vfio-pci"
              "sysfsdev=/sys/bus/mdev/devices/90ce754f-8bf8-488b-a693-73ed08cce2e9"
              "x-igd-opregion=on"
              "display=on"
              "driver=vfio-pci-nohotplug,ramfb=on"
              "xres=${toString config.virtualisation.resolution.x},yres=${toString config.virtualisation.resolution.y}"
            ]}"
          ];
        };
      };
    });
  }
  ```

</details>

[GVT-g]: https://nixos.wiki/wiki/IGVT-g
