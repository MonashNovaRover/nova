{ pkgs }:

with pkgs;

{
  nova-science = callPackage ./science { };
}
