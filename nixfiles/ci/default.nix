{ supportedSystems ? [ "x86_64-linux" "aarch64-linux" ]
, nixpkgs
, nova-monorepo
, declInput
, ...
}@args:

let
  nixfiles = nova-monorepo + "/nixfiles";

  inherit (import ./generation/variation args)
    mergeJobsetPlanners
    planPrJobsets
    planRosDistroJobsets;

  inherit (import ./generation/inputs.nix args)
    mkGitHubInput
    mkNovaInput
    novaInputs
    homeManagerInput
    jetpackNixosInput
    nixosHardwareInput;

  pkgs = import nixpkgs { };

  mkJobset = { description, nixexprpath, inputs ? { }, ... }@args: {
    enabled = 1;
    hidden = false;
    inherit description;
    nixexprinput = "nova-monorepo";
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
      nova-monorepo = mkNovaInput { repo = "nova"; };
      supportedSystems = {
        type = "nix";
        value = "[ \"${builtins.concatStringsSep "\" \"" supportedSystems}\" ]";
        emailresponsible = false;
      };
    } // inputs;
  };

  mkJobsets = builtins.mapAttrs (name: mkJobset);

  planRosDistroAndPrJobsets = mergeJobsetPlanners [ planRosDistroJobsets planPrJobsets ];

  jobsets =
    (mkJobsets (planRosDistroAndPrJobsets "workspaces" {
      description = "Nova Rover software";
      nixexprpath = "nixfiles/ci/jobsets/workspaces.nix";
      inputs = novaInputs;
    })) //
    (mkJobsets (planRosDistroAndPrJobsets "misc" {
      description = "Miscellaneous packages";
      nixexprpath = "nixfiles/ci/jobsets/misc.nix";
    })) //
    (mkJobsets (planRosDistroAndPrJobsets "tests" {
      description = "Tests";
      nixexprpath = "nixfiles/ci/jobsets/tests.nix";
      inputs = novaInputs;
    })) //
    {
      docs = mkJobset {
        description = "Nova Rover documentation";
        nixexprpath = "nixfiles/ci/jobsets/docs.nix";
        inputs = { home-manager = homeManagerInput; };
      };
      isos = mkJobset {
        description = "Nova Rover ISOs";
        nixexprpath = "nixfiles/ci/jobsets/isos.nix";
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
        nixexprpath = "nixfiles/ci/jobsets/docker.nix";
        inputs = { home-manager = homeManagerInput; };
        checkinterval = 60 * 60 * 24 * 7;
      };
      devices = mkJobset {
        description = "Team device configurations, prebuilt for binary cache convenience";
        nixexprpath = "nixfiles/ci/jobsets/devices.nix";
        inputs = novaInputs // {
          home-manager = homeManagerInput;
          jetpack-nixos = jetpackNixosInput;
        };
        checkinterval = 60 * 60 * 24 * 7;
      };
      slides = mkJobset {
        description = "Workshop slides";
        nixexprpath = "nixfiles/ci/jobsets/slides.nix";
        checkinterval = 60 * 60 * 24;
        inputs.slides = mkNovaInput { repo = "slides"; };
      };
    };
in
{
  jobsets = pkgs.writeText "jobset.json" (builtins.toJSON jobsets);
}
