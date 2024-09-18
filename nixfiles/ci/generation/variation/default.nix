{ nixpkgs
, ...
}@args:

let
  pkgs = import nixpkgs { };
in
import ./prs.nix args //
import ./ros.nix args //
rec {
  mergeJobsetPlanners = planners: name: args: builtins.foldl'
    (merged: planner:
      pkgs.lib.foldlAttrs
        (acc: name: plan: acc // planner name plan)
        { }
        (merged name args)
    )
    (builtins.elemAt planners 0)
    (pkgs.lib.drop 1 planners);
}
