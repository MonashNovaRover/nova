{ pkgs
, nixfiles
}:

{
  mkOptions = { module, specialArgs ? { } }:
    builtins.removeAttrs (pkgs.lib.evalModules {
      inherit specialArgs;
      modules = [
        { _module.check = false; }
        module
      ];
    }).options [ "_module" ];

  mkOptionsMd = options: (pkgs.nixosOptionsDoc {
    inherit options;
    documentType = "none";
    transformOptions = option:
      option // {
        declarations = map
          (declaration:
            let
              isLocalModule =
                if builtins.isPath nixfiles then
                  pkgs.lib.path.hasPrefix nixfiles (/. + declaration)
                else
                  pkgs.lib.hasPrefix (toString nixfiles) (toString declaration);
            in
            if isLocalModule then
              rec {
                name = (pkgs.lib.removePrefix ((toString nixfiles) + "/") declaration) +
                (pkgs.lib.optionalString (builtins.readFileType declaration == "directory") "/default.nix");
                url = "https://github.com/MonashNovaRover/nixfiles/blob/master/${name}";
              }
            else declaration
          )
          option.declarations;
      };
  }).optionsCommonMark;
}