{
  imports = [
    ./can_sleuth
    ./nova_cli
    ./nova_testing
  ] ++ (
    builtins.filter builtins.pathExists [
      ./coms_utils
      ./libblcmd
      ./libcanmd
    ]
  );
}
