{ supportedSystems ? [ "x86_64-linux" "aarch64-linux" ]
, nixpkgs
, src
, declInput
}:

let
  pkgs = import nixpkgs { };

  repos = [
    { repo = "rover"; branch = "feature/nix"; }
    { repo = "cameras2"; }
    { repo = "gui"; branch = "feature/nix"; }
    { repo = "coms_utils"; branch = "feature/nix"; }
  ];

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
  } // args // {
    inputs = {
      nixpkgs = mkGitHubInput { owner = "NixOS"; repo = "nixpkgs"; branch = "nixos-unstable"; };
      src = mkNovaInput { repo = "nixfiles"; };
      supportedSystems = {
        type = "nix";
        value = "[ ''${builtins.concatStringsSep "'' ''" supportedSystems}'' ]";
        emailresponsible = false;
      };
    } // inputs;
  };

  mkGitHubInput = { owner, repo, branch ? null }: {
    type = "git";
    value = "git@github.com:${owner}/${repo}.git${pkgs.lib.optionalString (branch != null) (" ${branch}")}";
    emailresponsible = false;
  };

  mkNovaInput = args: mkGitHubInput ({ owner = "MonashNovaRover"; } // args);

  jobsets = {
    workspaces = mkJobset {
      description = "Nova Rover workspaces";
      nixexprpath = "ci/release.nix";
      inputs = builtins.listToAttrs
        (map
          ({ repo, branch ? null }:
            pkgs.lib.nameValuePair
              repo
              (mkNovaInput { inherit repo branch; }))
          repos);
    };
  };
in
{
  jobsets =
    pkgs.writeText "jobset.json" (builtins.toJSON jobsets);
}
