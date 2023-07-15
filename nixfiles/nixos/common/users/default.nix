{ options, config, lib, ... }:

{
  options = {
    nova.users = lib.mkOption {
      type = with lib.types; attrsOf (submodule {
        options = {
          enable = lib.mkEnableOption "Enable the standard Monash Nova Rover user.";
        };
      });
      default = { };
      description = lib.mdDoc "Users that have access to Monash Nova Rover features.";
    };

    home-manager.nova.sharedModules = options.home-manager.sharedModules // {
      description = lib.mdDoc "Extra modules added to Nova Rover users.";
    };
  };

  config = {
    nova.users.nova.enable = lib.mkDefault false;

    users.users = lib.mkMerge [
      (builtins.mapAttrs
        (name: userConfig: lib.mkIf userConfig.enable {
          isNormalUser = true;
        })
        config.nova.users)

      {
        nova = lib.mkIf config.nova.users.nova.enable {
          description = "Monash Nova Rover";
          hashedPassword =
            "***REMOVED***";
          extraGroups = [
            config.users.groups.wheel.name
          ];
        };
      }
    ];

    home-manager = {
      useUserPackages = true;
      users =
        builtins.mapAttrs
          (name: userConfig: lib.mkIf userConfig.enable
            (lib.mkMerge config.home-manager.nova.sharedModules))
          config.nova.users;
    };

    assertions = [
      {
        assertion =
          let
            users = ((builtins.foldl'
              (names: name: if config.nova.users.${name}.enable then names ++ [ name ] else names)
              [ ]
              (builtins.attrNames config.nova.users)
            ) ++
            (builtins.foldl'
              (names: name: if config.users.users.${name}.group != "" then names ++ [ name ] else names)
              [ ]
              (builtins.attrNames config.users.users)));
          in
          builtins.all
            (user: builtins.elem user users)
            (builtins.attrNames config.home-manager.users);
        message = ''
          You have added a Home Manager user to the system without enabling it in nova.users or users.users!
          Ignore the other failed assersions for now, and fix this first.
        '';
      }
    ];
  };
}
