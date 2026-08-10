/*
Generate markdown from a NixOS/Home Manager option schema, with local declarations rewritten to GitHub links.
Inspired by Nixpkgs and rewritten:
https://github.com/NixOS/nixpkgs/blob/45ae0efbbce2aada6d5e8de6ace0c803b08ac9c7/doc/default.nix#L75-L86
      
*/
{ pkgs
, nova-monorepo
}:
let
  linkifyDeclarations = declarations:
    map
      (declaration:
        let
          isLocalModule =
            if builtins.isPath nova-monorepo then
              pkgs.lib.path.hasPrefix nova-monorepo (/. + declaration)
            else
              pkgs.lib.hasPrefix (toString nova-monorepo) (toString declaration);
        in
        if isLocalModule then
          rec {
            name = (pkgs.lib.removePrefix ((toString nova-monorepo) + "/") declaration) +
              (pkgs.lib.optionalString (builtins.readFileType declaration == "directory") "/default.nix");
            url = "https://github.com/MonashNovaRover/nova/blob/master/${name}";
            # TODO: Is there a way to generate URLs with a fixed Git revision?
            # Orlando thinks there might be a way: https://github.com/MonashNovaRover/nova/pull/1167#discussion_r3615129415
          }
        else declaration
      )
      declarations;
in
{ module, specialArgs ? { } }:
  (pkgs.nixosOptionsDoc {
    options = builtins.removeAttrs (pkgs.lib.evalModules {
      inherit specialArgs;
      modules = [
        { _module.check = false; }
        module
      ];
    }).options [ "_module" ];
    documentType = "none";
    transformOptions = option:
      option // {
        declarations = linkifyDeclarations option.declarations;
      };
  }).optionsCommonMark
