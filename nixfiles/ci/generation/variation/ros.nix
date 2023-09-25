{ nixpkgs
, ...
}@args:

let
  pkgs = import nixpkgs { };

  mkRosDistroInput = rosDistro: {
    type = "nix";
    value = "${if rosDistro == null then "null" else "\"${rosDistro}\""}";
    emailresponsible = false;
  };
in
{
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
}
