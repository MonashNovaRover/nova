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


  categoryModule.options = {
    nova = lib.mkOption {
      type = packageSetType;
      default = null;
      description = lib.mdDoc "Nova Rover packages.";
    };
    other = lib.mkOption {
      type = packageSetType;
      default = null;
      description = lib.mdDoc "Third-party packages.";
    };
  };
in
{
  options = {
    packages = lib.mkOption {
      type = with lib.types; submodule categoryModule;
      default = { };
      description = "Regular packages.";
    };
    pythonPackages = lib.mkOption {
      type = with lib.types; submodule categoryModule;
      default = { };
      description = "Python packages.";
    };
    rosPackages = lib.mkOption {
      type = with lib.types; submodule categoryModule;
      default = { };
      description = "ROS packages.";
    };
  };
}
