{ ... }:
{
  # Enables nix-ld to allow nova-unity-sim to find runtime-dependencies
  config = {
    programs.nix-ld.enable = true;
  };
}
