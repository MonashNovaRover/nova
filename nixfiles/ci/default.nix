{ supportedSystems ? [ "x86_64-linux" "aarch64-linux" ]
, nixpkgs
, nixfiles
, declInput
, ...
}@args:

let
  pkgs = import nixpkgs { };
  allNovaRepos = builtins.foldl' pkgs.lib.recursiveUpdate { } (builtins.attrValues (import ./nova-repos.nix));

  mkJobset = { description, nixexprpath, inputs ? { }, ... }@args: {
    enabled = 1;
    hidden = false;
    inherit description;
    nixexprinput = "nixfiles";
    inherit nixexprpath;
    checkinterval = 60;
    schedulingshares = 100;
    enableemail = false;
    enable_dynamic_run_command = false;
    emailoverride = "";
    keepnr = 1;
  } // args // {
    inputs = {
      nixpkgs = mkGitHubInput { owner = "NixOS"; repo = "nixpkgs"; branch = "nixos-unstable"; };
      nixfiles = mkNovaInput { repo = "nixfiles"; };
      supportedSystems = {
        type = "nix";
        value = "[ \"${builtins.concatStringsSep "\" \"" supportedSystems}\" ]";
        emailresponsible = false;
      };
    } // inputs;
  };

  mkJobsets = builtins.mapAttrs (name: mkJobset);

  mkGitHubInput = { owner, repo, branch ? null }: {
    type = "git";
    value = "git@github.com:${owner}/${repo}.git${pkgs.lib.optionalString (branch != null) (" ${branch}")}";
    emailresponsible = false;
  };

  mkNovaInput = args: mkGitHubInput ({ owner = "MonashNovaRover"; } // args);

  novaInputs = builtins.mapAttrs
    (repo: branch: mkNovaInput { inherit repo branch; })
    allNovaRepos;

  novaPrs = builtins.foldl'
    (allPrs: repo:
      let
        branch = allNovaRepos.${repo} or null;
        repoPrs = builtins.attrValues (builtins.fromJSON (builtins.readFile args."${repo}-pr-json"));
      in
      allPrs ++ builtins.filter
        (pr:
          # We're only interested in open PRs.
          pr.state == "open" &&

          # Ensure that the PR is targeting the expected branch, or the default branch if none is specified.
          (pr.base.ref == (if branch == null then pr.base.repo.default_branch else branch)))
        repoPrs)
    [ ]
    ([ "nixfiles" ] ++ builtins.attrNames allNovaRepos);

  homeManagerInput = mkGitHubInput {
    owner = "nix-community";
    repo = "home-manager";
  };

  jetpackNixosInput = mkGitHubInput {
    owner = "anduril";
    repo = "jetpack-nixos";
  };

  nixosHardwareInput = mkGitHubInput {
    owner = "NixOS";
    repo = "nixos-hardware";
  };

  mkRosDistroInput = rosDistro: {
    type = "nix";
    value = "${if rosDistro == null then "null" else "\"${rosDistro}\""}";
    emailresponsible = false;
  };

  planRosDistroJobsets = name: { description, inputs ? { }, ... }@args:
    let extraDistros = [ "foxy" ];
    in
    { ${name} = args // { inputs = inputs // { rosDistro = mkRosDistroInput null; }; }; }
    // builtins.listToAttrs (map
      (rosDistro: pkgs.lib.nameValuePair "${name}-${rosDistro}" (args // {
        description = "${description} (for ${pkgs.lib.toUpper (builtins.substring 0 1 rosDistro)}${builtins.substring 1 (builtins.stringLength rosDistro) rosDistro})";
        inputs = inputs // { rosDistro = mkRosDistroInput rosDistro; };
      }))
      extraDistros);

  planPrJobsets = name: { description, inputs ? { }, ... }@args:
    let
      # Exclude PRs for repositories that aren't used in the jobset.
      repoWhitelist = [ "nixfiles" ] ++ builtins.attrNames inputs;
      relevantPrs = builtins.filter (pr: builtins.elem pr.base.repo.name repoWhitelist) novaPrs;
    in
    { ${name} = args; }
    // builtins.listToAttrs (map
      (pr: pkgs.lib.nameValuePair "${name}-pr-${pr.base.repo.name}-${pr.number}" (args // {
        hidden = true;
        description = "${description} - ${pr.base.repo.name}#${toString pr.number} (${pr.title})";
        inputs = inputs // {
          # Replace the input in question with the PR's merge ref.
          ${pr.base.repo.name} = mkGitHubInput {
            owner = pr.head.repo.owner.login;
            repo = pr.head.repo.name;
            branch = "pull/${pr.number}/merge";
          };

          # Add the link as an input, for convenience in the Web UI.
          "_pr-link" = {
            type = "string";
            value = pr.html_url;
            emailresponsible = false;
          };
        };
      }))
      relevantPrs);

  planRosDistroAndPrJobsets = name: args: pkgs.lib.foldlAttrs
    (acc: name: plan: acc // planPrJobsets name plan)
    { }
    (planRosDistroJobsets name args);

  jobsets =
    (mkJobsets (planRosDistroAndPrJobsets "workspaces" {
      description = "Nova Rover software";
      nixexprpath = "ci/jobsets/workspaces.nix";
      inputs = novaInputs;
    })) //
    (mkJobsets (planRosDistroAndPrJobsets "misc" {
      description = "Miscellaneous packages";
      nixexprpath = "ci/jobsets/misc.nix";
    })) //
    {
      docs = mkJobset {
        description = "Nova Rover documentation";
        nixexprpath = "ci/jobsets/docs.nix";
        inputs = { home-manager = homeManagerInput; };
      };
      isos = mkJobset {
        description = "Nova Rover ISOs";
        nixexprpath = "ci/jobsets/isos.nix";
        inputs = novaInputs // {
          nixpkgs-stable = mkGitHubInput { owner = "NixOS"; repo = "nixpkgs"; branch = "nixos-23.05"; };
          home-manager = homeManagerInput;
          jetpack-nixos = jetpackNixosInput;
          nixos-hardware = nixosHardwareInput;
        };
        checkinterval = 60 * 60 * 24 * 7;
      };
      docker = mkJobset {
        description = "Docker images";
        nixexprpath = "ci/jobsets/docker.nix";
        inputs = { home-manager = homeManagerInput; };
        checkinterval = 60 * 60 * 24 * 7;
      };
      devices = mkJobset {
        description = "Team device configurations, prebuilt for binary cache convenience";
        nixexprpath = "ci/jobsets/devices.nix";
        inputs = novaInputs // {
          home-manager = homeManagerInput;
          jetpack-nixos = jetpackNixosInput;
        };
        checkinterval = 60 * 60 * 24 * 7;
      };
      slides = mkJobset {
        description = "Workshop slides";
        nixexprpath = "ci/jobsets/slides.nix";
        checkinterval = 60 * 60 * 24;
        inputs.slides = mkNovaInput { repo = "slides"; };
      };
    };
in
{
  jobsets = pkgs.writeText "jobset.json" (builtins.toJSON jobsets);
}
