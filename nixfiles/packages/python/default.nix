{ callPackage }:

{
  jcan = callPackage ./jcan { };
  mkdocs-safe-text-plugin = callPackage ./mkdocs-safe-text-plugin { };
}
