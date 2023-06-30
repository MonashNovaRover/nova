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
      default = { };
      description = "Regular packages.";
    };
    pythonPackages = lib.mkOption {
      type = packageSetType;
      default = { };
      description = "Python packages.";
    };
    rosPackages = lib.mkOption {
      type = packageSetType;
      default = { };
      description = "ROS packages.";
    };
  };
}
