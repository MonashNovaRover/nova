# Creating a new ROS Package

To create and use a new ROS package, the following steps must be taken. It is assumed that you have the `nixfiles` repository in `~/nova/nixfiles`, and the source directory in `~/nova/src` with a symbolic link pointing to it in `~/nova/nixfiles/external`.

1. If you’re making a new repository, create a new directory for the package in `~/nova/src/ros`.
2. Add or update the directory’s `default.nix` file, with contents like the following:

`~/nova/src/ros/example/default.nix`
```nix
{
  rosPackages = pkgs: with pkgs; {
    nova-example = callPackage ./nix/packages/example { };
  };
}
```

(The format of external package directories is [documented in full in the `nixfiles` repository](https://github.com/MonashNovaRover/nixfiles/blob/master/doc/nix.md#adding-out-of-tree-packages).)

3. Add a minimal Nix expression for the new package.

`~/nova/src/ros/example/nix/packages/example/default.nix`
```nix
{ buildRosPackage
}:

buildRosPackage {
  name = "example";
  buildType = "ament_cmake"; # Or "ament_python"!
}
```

4. Add the package to the list of team packages in `~/nova/nixfiles/packages/ros/nova-workspace/default.nix`.
5. Enter the package development environment:
```
nix-shell ~/nova/nixfiles -A env.nova-example
```

```
ros2 pkg create example \
--destination-directory ~/nova/src/ros/example \
--build-type ament_cmake \
--maintainer-name 'Monash Nova Rover' \
--maintainer-email 'novaroverteam@monash.edu'
```

6. Create the package source files. The build type can be either `ament_cmake` for a C++ or interface package or `ament_python` for a Python package.
7. Add the source directory and build type to the package `default.nix`.

```nix
{ lib
, buildRosPackage
}:

buildRosPackage {
name = "example";
build_type = "ament_cmake";

src = builtins.path rec {
name = "example-source";
path = ../../../example;
filter = lib.novaSourceFilter [ ] path;
};
}
```

8. To develop, first exit and re-enter the development shell. This must be done whenever the default.nix is changed, to make new dependencies available.
