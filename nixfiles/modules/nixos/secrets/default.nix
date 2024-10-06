{ pkgs, config, lib, ... }:

{
  imports = [
    "${builtins.fetchTarball {
      url = "https://github.com/Mic92/sops-nix/archive/aff2f88277dabe695de4773682842c34a0b7fd54.tar.gz";
      sha256 = "sha256:0r8gw0ijnm8z6ckpvix2y7c5ym0cxvf8zjim966il9dd2aqj6fzn";
    }}/modules/sops"
  ];

  sops = {
    age.keyFile = "/var/lib/sops-nix/keys.txt";
    defaultSopsFile = ./secrets.yaml; #this will add secrets.yaml to the store, do we want it in the store?
    defaultSopsFormat = "yaml";

    secrets = lib.mkMerge [
      {
        "hydra/password" = { };
        "nova/hashed_password" = {
          neededForUsers = true;
        };
      }

      # These secrets are only needed by CI
      (lib.mkIf config.nova.ci.master.enable {
        "hydra/git_ssh_key" = { owner = "hydra"; group = "hydra"; };
        "hydra/binary_cache_secret" = { owner = "hydra"; group = "hydra"; };
        "hydra/github_token"= { owner = "hydra"; group = "hydra"; };
      })
    ];
  };
}