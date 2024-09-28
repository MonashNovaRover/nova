{ supportedSystems
, nixpkgs
, nova-monorepo
, ...
}@args:

let
  nixfiles = nova-monorepo + "/nixfiles";
  lib = import ../lib.nix args;

  pkgs = import <nixpkgs> { overlays = [
    (import (nixfiles + "/overlay"))
    (self: super: import (nixfiles + "/packages/other") { inherit (self) callPackage; })
  ]; };

  evaledSource = builtins.path rec {
    name = "arm-interfaces-source";
    path = ../../external/src/ros/rover/arm/arm_interfaces;
    filter = (path: type: true);
  };
in
  throw evaledSource

# [ /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/other/coms_utils
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/other/libblcmd
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/other/libcanmd
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/ros/cameras2
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/ros/gui
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/ros/nova-gui
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/ros/rover ]

# [ /nix/store/6pzhkxnh5yc4zj89z05j5c5wjwh769m5-source/nixfiles/external/src/ros/rover/test.nix
#   /nix/store/6pzhkxnh5yc4zj89z05j5c5wjwh769m5-source/nixfiles/external/src/ros/rover/nix/packages/arm-interfaces «thunk» ]

# import ~/nova/nixfiles/ci/jobsets/test.nix { nixpkgs = <nixpkgs>; nova-monorepo = ./nova; supportedSystems = null; }