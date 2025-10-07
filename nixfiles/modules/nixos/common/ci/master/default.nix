{ options, config, pkgs, lib, ... }:

let
  cfg = config.nova.ci.master;
in
{
  options.nova.ci.master = {
    enable = lib.mkEnableOption "CI master services";
    domain = lib.mkOption {
      type = with lib.types; str;
      description = "The primary domain name of the server.";
      default = "localhost";
    };
    hydra = {
      localMaxJobs = lib.mkOption {
        type = with lib.types; ints.unsigned;
        description = "The maximum number of local build jobs to run concurrently.";
        default = 0;
      };
      localSpeedFactor = lib.mkOption {
        type = with lib.types; int;
        description = "The relative speed of this builder. Higher is faster.";
        default = 1;
      };
      subdomain = lib.mkOption {
        type = with lib.types; str;
        description = "The subdomain that Hydra is served on.";
        default = "hydra";
      };
      githubToken = lib.mkOption {
        type = with lib.types; nullOr str;
        description = "The GitHub token used to authenticate with the GitHub API. ";
        default = null;
      };
    };
    gitSSHKey = lib.mkOption {
      type = with lib.types; path;
      description = "The SSH key uses to clone Git repositories.";
    };
    cacheSecretKey = lib.mkOption {
      type = with lib.types; path;
      description = "The secret key used to sign binary cache artifacts.";
    };
  };

  config = lib.mkIf cfg.enable {
    nova.ci.common.enable = true;

    services.hydra = {
      enable = true;
      package = (options.services.hydra.package.default.override { nix = pkgs.nixVersions.nix_2_20; }).overrideAttrs ({ patches ? [ ], ... }: {
        # https://github.com/NixOS/hydra/pull/1359 completely broke the .narinfo
        # server.
        version = "2024-03-08";
        src = pkgs.fetchFromGitHub {
          owner = "NixOS";
          repo = "hydra";
          rev = "8f56209bd6f3b9ec53d50a23812a800dee7a1969";
          hash = "sha256-mhEj02VruXPmxz3jsKHMov2ERNXk9DwaTAunWEO1iIQ=";
        };
        patches = patches ++ [
          # https://github.com/NixOS/hydra/security/advisories/GHSA-2p75-6g9f-pqgx
          (pkgs.fetchpatch {
            name = "CVE-2024-32657.patch";
            url = "https://github.com/NixOS/hydra/commit/b72528be5074f3e62e9ae2c2ae8ef9c07a0b4dd3.patch";
            hash = "sha256-KqX7fJGxJXpj5OdVp7Cc/XWMlrkTjTztoSHJhJanpgI=";
          })
        ];
      });
      listenHost = "localhost";
      hydraURL =
        if (lib.hasPrefix "localhost" cfg.domain)
        then "http://${cfg.domain}"
        else "https://${cfg.hydra.subdomain}.${cfg.domain}";
      notificationSender = "nova@monash.edu";
      useSubstitutes = true;
      buildMachinesFiles =
        (lib.optional (config.nix.buildMachines != [ ]) "/etc/nix/machines") ++
        (lib.optional (cfg.hydra.localMaxJobs != 0) (pkgs.writeText "hydra-build-machines" ''
          localhost ${builtins.concatStringsSep "," ([ builtins.currentSystem ] ++ config.nix.settings.extra-platforms or [ ])} - ${toString cfg.hydra.localMaxJobs} ${toString cfg.hydra.localSpeedFactor} ${builtins.concatStringsSep "," [ "nixos-test" "big-parallel" ]}
        ''));
      logo = pkgs.nova.nova-icons + /share/icons/hicolor/512x512/apps/nova-logo-white-and-orange.png;
      extraConfig = ''
        binary_cache_secret_key_file = ${cfg.cacheSecretKey}
        binary_cache_public_uri = ${config.services.hydra.hydraURL}

        # Increase the maximum output size (useful for things like ISO images)
        max_output_size = 34359738368

        <dynamicruncommand>
          enable = 1
        </dynamicruncommand>
        <git-input>
          timeout = 3600
        </git-input>

        ${lib.optionalString (cfg.hydra.githubToken != null) ''
          <github_authorization>
            MonashNovaRover = Bearer ${cfg.hydra.githubToken}
          </github_authorization>

          <githubstatus>
            jobs = nova:workspaces(?:-pr-.*-\d+)?:(?!.*-inputs).*
            useShortContext = 1
            inputs = nova
            ${builtins.concatStringsSep "\n" (map
              (repo: "inputs = ${repo}")
              (builtins.foldl' (repos: category: repos ++ builtins.attrNames category) [ ] (builtins.attrValues (import ../../../../../ci/nova-repos.nix))))}
          </githubstatus>
        ''}
      '';
    };

    # https://github.com/NixOS/hydra/issues/1186
    systemd.services.hydra-evaluator.environment.GC_DONT_GC = "true";

    systemd.services.hydra-setup = {
      description = "Set up Hydra";
      wantedBy = [ "multi-user.target" ];
      requires = [ "hydra-init.service" ];
      after = [ "hydra-init.service" ];
      path = [ config.services.hydra.package ];
      serviceConfig = {
        Type = "oneshot";
        RemainAfterExit = true;
        inherit (config.systemd.services.hydra-init.serviceConfig) User;
      };
      script = ''
        # Create user accounts
        hydra-create-user nova \
          --full-name 'Monash Nova Rover' \
          --email-address 'novaroverteam@monash.edu' \
          --password-hash '$argon2id$v=19$m=262144,t=3,p=1$aGNmTld2SnFjYWduQUtaTQ$CAuk99WA+wQxeGCIX4hoLQ' \
          --role admin

        # Configure SSH
        install -Dm600 ${cfg.gitSSHKey} ~/.ssh/id_rsa
        install -Dm600 ${pkgs.writeText "known_hosts" ''
          github.com ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABgQCj7ndNxQowgcQnjshcLrqPEiiphnt+VTTvDP6mHBL9j1aNUkY4Ue1gvwnGLVlOhGeYrnZaMgRK6+PKCUXaDbC7qtbW8gIkhL7aGCsOr/C56SJMy/BCZfxd1nWzAOxSDPgVsmerOBYfNqltV9/hWCqBywINIR+5dIg6JTJ72pcEpEjcYgXkE2YEFXV1JHnsKgbLWNlhScqb2UmyRkQyytRLtL+38TGxkxCflmO+5Z8CSSNY7GidjMIZ7Q4zMjA2n1nGrlTDkzwDCsw+wqFPGQA179cnfGWOWRVruj16z6XyvxvjJwbz0wQZ75XK5tKSb7FNyeIEs4TT4jk+S4dhPeAUC5y+bDYirYgM4GC7uEnztnZyaVWQ7B381AK4Qdrwt51ZqExKbQpTUNn+EjqoTwvqNj4kqx5QUCI0ThS/YkOxJCXmPUWZbhjpCg56i+2aB6CmK2JGhn57K5mj0MNdBXA4/WnwH6XoPWJzK5Nyu2zB3nAZp+S5hpQs+p1vN1/wsjk=
          github.com ecdsa-sha2-nistp256 AAAAE2VjZHNhLXNoYTItbmlzdHAyNTYAAAAIbmlzdHAyNTYAAABBBEmKSENjQEezOmxkZMy7opKgwFB9nkt5YRrYMjNuG5N87uRgg6CLrbo5wAdT/y6v0mKV0U2w0WZ2YB/++Tpockg=
          github.com ssh-ed25519 AAAAC3NzaC1lZDI1NTE5AAAAIOMqqnkVzrm0SdG6UOoqKLsabgH5C9okWi0dh2l9GKJl
        ''} ~/.ssh/known_hosts
      '';
    };

    services.caddy = lib.mkIf (!(lib.hasPrefix "localhost" cfg.domain)) {
      enable = true;
      virtualHosts.hydra = {
        hostName = "${cfg.hydra.subdomain}.${cfg.domain}";
        extraConfig = ''
          basicauth {
            nova $2a$14$4EcarI150GjEOzkmgmTS2eynl4P5XBNcr9n3tDV8P2igqgLyrFtky
          }

          reverse_proxy :${toString config.services.hydra.port}

          @doc_matcher vars_regexp doc {http.request.orig_uri.path} ^\/manual\/(.+?)(?:\/|$)(.*)$
          rewrite @doc_matcher /job/nova/docs/{re.doc.1}/latest/download-by-type/doc/manual/{re.doc.2}
        '';
      };
    };

    networking.firewall.allowedTCPPorts = [ 80 443 ];
  };
}
