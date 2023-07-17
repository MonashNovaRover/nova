# NixOS virtualisation

NixOS systems does not have to activate on bare metal; they can just as easily
activate in containers and virtual machines.

This repository contains prebuilt configurations for use in both containers and
virtual machines.

## Advantages and disadvantages of containers and VMs

Containers and virtual machines each have their own advantages and
disadvantages, particularly in the context of NixOS.

| Feature | Container | Virtual machine |
| :------ | :-------: | :-------------: |
| Startup time | **Fast** | Slow |
| Resource usage | **Low** | High |
| Isolation | Low (see the [warning][nixos-container]) | **High** |
| System integration | **High** | Low |
| Re-activation from host (like `nixos-rebuild switch`) | **Fast** | Slow |
| Emulated kernel | No | **Yes** |
| Accurate boot process | No (skips bootloader and NixOS stage 1) | **Partial** |
| Hardware-accelerated graphics | No | **[With effort](https://nixos.wiki/wiki/IGVT-g)** |

## Using containers

### Creating containers

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
   $ extra-container create --expr 'import ./nixos/installer/container { }'
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

### Updating containers

Containers can be updated with a new system configuration at runtime, through
the same mechanism used by `nixos-rebuild switch`. The container configuration
can also be modified after creation (though this will often require the
container to be restarted to apply).

To do update or modify containers, run the creation command again with the same
name. Use the `--update-changed` (`-u`) flag to active the new system
configuration, and/or the `--restart-changed` (`-r`) flag to restart the
container.

### Container customisation

`./nixos/installer/container` can be called with the following arguments:

- `name`: The name of the container (`nova` by default)
- `modules`: A list of extra NixOS modules to add to the container (empty by default)

For example:

```
$ extra-container create --expr 'import ./nixos/installer/container {
  name = "nova-headless";
  modules = [{
    services.xserver.enable = false;
    nova.desktop.enable = false;
  }];
}'
```

[nixos-container]: https://nixos.org/manual/nixos/unstable/index.html#ch-containers
[extra-container]: https://github.com/erikarvstedt/extra-container