# Using the Nix packages

## Preparation

1. Use any Linux distro. macOS will in theory work to some extent, but is untested. Some of our software requires Linux.
2. Install [Nix](https://nixos.org/).
3. Configure Nix to use the [binary caches](./caches.md). This is optional, but highly recommended to cut down on build times.
4. In this repository directory, run the [checkout script](./scripts/checkout-nova-sources.sh).
5. Make sure the repositories cloned to [external/src](./external/src) are on up-to-date branches with a `default.nix`.

Any commands in this README starting with `nix-shell`, `nix-build`, etc.
should be started in the top-level repository directory.

## Execution

There are two alternate ways to enter the Nova Rover ROS workspace.

---

To open a temporary shell in the workspace, run the following command:

```
nix-shell -p 'with import ./. { }; pkgs.ros.nova-workspace'
```

Note that the workspace includes all Nova Rover packages by default, which can
lead to long evaluation and build times. The GUI frontend is one of the [worst
offenders](https://www.reddit.com/r/ProgrammerHumor/comments/6s0wov).

To create a workspace with a specific set of packages, the `novaPackages`
argument can be set. For example:

```
nix-shell -p 'with import ./. { }; pkgs.ros.nova-workspace.override {
  novaPackages = with pkgs.ros; [
   nova-core
   nova-control
  ];
}'
```

---

Alternatively, for something a little more permanent, you can build the
workspace and manually add `result/bin` to your `PATH`. `result` is a symbolic
link that gets updated whenever the workspace package is built. Its existance
will also prevent Nix from garbage collecting it or any dependencies.

```
nix-build -A pkgs.ros.nova-workspace

export PATH="$PWD/result/bin:$PATH"
# Add something similar to your shell init script.
```

As is explained in the shell example, it is often beneficial to build the
workspace with a small subset of Nova Rover packages. This can be done like so:

```
nix-build -E 'with import ./. { }; pkgs.ros.nova-workspace.override {
  novaPackages = with pkgs.ros; [
   nova-core
   nova-control
  ];
}'
```
---

After you are in the workspace, run the following command, which configures
things like shell completion.

```
eval "$(mk-nova-shell-setup)"
```

Now, the workspace can be used like normal. `ros2 run` to your heart's content.

> Whenever you change any source code, you can re-enter the shell or rebuild the
> workspace to use the new version. Only the packages that changed will be
> rebuilt.
>
> Note that this is not recommended for general development due to the lack of
> incremental compilation in individual packages. Read on for development
> instructions.

## Development

Development can be done in two styles:

1. Using [colcon](https://colcon.readthedocs.io/en/released/) as normal
   (recommended when working on multiple packages at once).
2. Using build tools like CMake directly (recommended when working on just one package).

### Colcon

1. Enter the workspace development shell.
   ```
   nix-shell -A pkgs.ros.nova-workspace.env
   ```

2. Switch to a workspace directory.
   ```
   cd external/src
   ```

3. Build the workspace.
   ```
   colcon build
   ```

4. Hack away.

### Direct

1. Enter a package development shell. For example, `control`:
   ```
   nix-shell -A pkgs.ros.nova-control
   ```

2. Switch to the package directory.
   ```
   cd external/src/ros/rover/control
   ```

3. Build the package with regular build tools.
   Note that some CMake packages will need `-DBUILD_TESTING=OFF`.
   ```
   mkdir -p build
   cd build
   cmake .. -DBUILD_TESTING=OFF
   cmake --build .
   ```

4. IDEs such as CLion will be able to work with the package as a regular CMake
   project, so long as they are started from the shell.

   When using CLion, make sure to manually enter the build tool executable names
   in a toolchain profile. This will force the IDE to use the tools in `PATH`
   instead of using incorrect autodetection results.

   ![](/doc/images/clion_toolchain_setup.png)

## Structure

Importing this repository in Nix will create the entrypoint function.

**Arguments (in an attribute set):**

`pkgs`: Optional. An instance of [Nixpkgs](https://github.com/NixOS/nixpkgs). This is used only to download pinned revisions of it and other package sources.

`repos`: Optional. A list of paths to out-of-tree Nova Rover software repositories. (Tip: [Use `--arg` to set this on the CLI.](https://nixos.org/manual/nix/unstable/command-ref/opt-common.html#opt-arg))

**Return values (in an attribute set):**

`pkgs`: An instance of a pinned (non-updating) revision of Nixpkgs with additional packages added.

### Using ROS packages

ROS packages can be accessed through either the `rosPackages` set or the `ros` alias (equivalent to `rosPackages.${defaultVersion}`).

Here are some examples:

- `pkgs.ros.nova-workspace` - The main workspace, with ROS 2 Foxy
- `pkgs.rosPackages.humble.nova-control` - The `control` package, built against ROS 2 Humble
- `pkgs.rosPackages.rolling.ros-core` - The [core variant](https://ros.org/reps/rep-2001.html#id32) of ROS 2 Rolling

### Adding out-of-tree packages

Out-of-tree packages are expected to be structured like so:

```
├── default.nix
├── nix
│  └── packages
│     ├── <name>
│     │  └── default.nix
│     ├── <name>
│     │  └── default.nix
│     └── ... (additional packages)
└── ... (the rest of the repository)
```

`default.nix` is a NixOS-style module that configures this repository, primarily to add packages.

There are three options:

- `packages`: Regular packages to add.
- `pythonPackages`: Python packages to add.
- `rosPackages`: ROS packages to add.

A typical `default.nix` would look like this:

```nix
{
  rosPackages = pkgs: with pkgs; {
    nova-my-ros-package = callPackage ./nix/packages/my-ros-package { };
  };
}
```

Consult existing repositories for practical examples.