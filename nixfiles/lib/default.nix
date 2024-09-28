pkgs: lib:

let
  inherit (pkgs)
    nix-gitignore
    github-gitignore;
in
{
  # A filter for a Nova Rover source tree, removing:
  #  - Version control files
  #  - Editor configuration files
  #  - Temporary files related to languages and frameworks
  #  - default.nix, shell.nix and nix/
  #
  # These files are not actually needed to build software,
  # but they influence the input hash, so they should be removed.
  #
  # Items in extraPatterns are added to the list of gitignore patterns.
  novaSourceFilter = extraPatterns: root: nix-gitignore.gitignoreFilterPure
    lib.cleanSourceFilter
    (
      [
        # Nix files in standard locations
        "default.nix"
        "shell.nix"
        "nix/" # This filters everything on the mono-repo!!!

        # IDE configuration files have no impact on build outputs.
        ".idea/"
        ".vscode/"
      ]

      ++ builtins.foldl'
        # While gitignoreFilterPure does support paths in the pattern list, it
        # is not possible to turn a derivation output path string into a true
        # path type.
        #
        # This is because Nix has no way to associate paths with derivations
        # like it does with strings, so when the path is used, Nix will not
        # know to build the derivation beforehand.
        #
        # The logic from gitignoreCompileIgnore to read patterns from a file
        # is therefore replicated here.
        (patterns: file: patterns ++ lib.toList (builtins.readFile file))
        [ ]
        ([
          # Languages and frameworks
          "${github-gitignore}/C.gitignore"
          # "${github-gitignore}/C++.gitignore # Disabled to reduce evaluation time. Almost completely a subset of C.gitignore".
          "${github-gitignore}/Python.gitignore"
          "${github-gitignore}/CMake.gitignore"
          # "${github-gitignore}/ROS.gitignore" # Faulty - filters out certain message files.
          "${github-gitignore}/community/ROS2.gitignore"
          "${github-gitignore}/community/Nix.gitignore"

          # Operating systems and editors
          "${github-gitignore}/Global/Linux.gitignore"
          # "${github-gitignore}/Global/macOS.gitignore" # Disabled to reduce evaluation time.
          "${github-gitignore}/Global/Vim.gitignore"
          "${github-gitignore}/Global/JetBrains.gitignore"
          "${github-gitignore}/Global/VisualStudioCode.gitignore"
        ])
      ++ lib.optional (builtins.pathExists (root + /.gitignore)) (root + /.gitignore)
      ++ extraPatterns
    )
    root;
}
