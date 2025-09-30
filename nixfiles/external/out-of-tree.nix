{ lib, ... }:

let
  packageSetType = lib.mkOptionType {
    name = "package-set";
    description = "package set";
    check = value: (isNull value) || (builtins.isFunction value);
    merge = loc: defs: builtins.foldl'
      (acc: curr:
        if (curr == null) then
          acc
        else
          pkgs: (acc pkgs) // (curr pkgs))
      (pkgs: { })
      (lib.getValues defs);
  };
in
{
  options = {
    packages = lib.mkOption {
      type = packageSetType;
      default = null;
      description = "Regular packages.";
    };
    pythonPackages = lib.mkOption {
      type = packageSetType;
      default = null;
      description = "Python packages.";
    };
    rosPackages = lib.mkOption {
      type = packageSetType;
      default = null;
      description = "ROS packages.";
    };

    shellAliases = lib.mkOption {
      type = with lib.types; attrsOf str;
      default = { };
      description = ''
        An attribute set that maps aliases (the top level attribute names
        in this option) to command strings or directly to build outputs.

        This option should only be used to manage simple aliases that are
        compatible across all shells.
      '';
    };
  };
}
