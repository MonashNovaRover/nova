{ nixpkgs
, src
, rover
, cameras2
, gui
, coms_utils
}:

let
  nova = import src {
    repos = [
      rover
      cameras2
      gui
      coms_utils
    ];
  };
in
rec {
  workspace = nova.pkgs.ros.nova-workspace;
  workspace-env-inputs = workspace.env.inputDerivation;
}
