{
  imports = [
    ./can_sleuth
    ./nova_cli
    ./nova_testing
  ] ++ (
    builtins.filter (p: builtins.pathExists (p + "/default.nix")) [
      ./coms_utils
      ./libblcmd
      ./libcanmd
    ]
  );
}
