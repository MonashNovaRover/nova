self: super:

{
  rosPackages = super.rosPackages // {
    appendDistroOverlay = rosOverlay: rosPackages:
      rosPackages // builtins.mapAttrs
        (rosDistro: rosPkgs:
          if rosPkgs ? overrideScope
          then rosPkgs.overrideScope rosOverlay
          else rosPkgs)
        rosPackages;
  };
}
