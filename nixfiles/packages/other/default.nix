{ callPackage }:

{
  github-gitignore = callPackage ./github-gitignore { };
  novafox = callPackage ./novafox { };
  nova-backgrounds = callPackage ./nova-backgrounds { };
  nova-icons = callPackage ./nova-icons { };
}
