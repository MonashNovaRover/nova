{ nixpkgs
, src
, declInput
}:

let
  pkgs = import nixpkgs { };

  mkJobset = { description, nixexprpath, inputs ? { }, ... }@args: {
    enabled = 1;
    hidden = false;
    inherit description;
    nixexprinput = "src";
    inherit nixexprpath;
    checkinterval = 60;
    schedulingshares = 100;
    enableemail = false;
    enable_dynamic_run_command = false;
    emailoverride = "";
    keepnr = 3;
    inputs = {
      nixpkgs = mkGitHubInput "NixOS" "nixpkgs";
      src = mkNovaInput "nixfiles";
    } // inputs;
  } // args;

  mkGitHubInput = owner: repo: {
    type = "git";
    value = "git@github.com:${owner}/${repo}.git";
    emailresponsible = false;
  };

  mkNovaInput = mkGitHubInput "MonashNovaRover";
in
pkgs.writeText "jobset.json" (builtins.toJSON {
  workspaces = mkJobset {
    description = "Nova Rover workspaces";
    nixexprpath = "release.nix";
    inputs = pkgs.lib.genAttrs [
      "rover"
      "cameras2"
      "gui"
      "coms_utils"
    ]
      mkNovaInput;
  };
})
