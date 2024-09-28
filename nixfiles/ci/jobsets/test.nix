{ supportedSystems
, nixpkgs
, nova-monorepo
, ...
}@args:

let
  nixfiles = nova-monorepo + "/nixfiles";
  lib = import ../lib.nix args;
in
  #throw lib.repos
  throw ([ ../../external/src/ros/rover/test.nix ] ++ (import ../../external/src/ros/rover/test.nix))

# [ /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/other/coms_utils
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/other/libblcmd
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/other/libcanmd
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/ros/cameras2
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/ros/gui
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/ros/nova-gui
# /nix/store/w4qxnwr7vj3f45wf7bx62zsashg9pl0i-source/nixfiles/external/src/ros/rover ]