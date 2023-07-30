pkgs: lib:

let
  inherit (pkgs)
    nix-gitignore
    github-gitignore;
in
{
  # Filters a Nova Rover source tree, removing:
  #  - Version control files
  #  - Editor configuration files
  #  - Temporary files related to languages and frameworks
  #  - default.nix, shell.nix and nix/
  #
  # These files are not actually needed to build software,
  # but they influence the input hash, so they should be removed.
  #
  # Items in extraPatterns are added to the list of gitignore patterns.
  cleanNovaSource = extraPatterns: src:
    # Note: These filtering operations were experimentally found to add an
    # average of 1.8 seconds to the evaluation time of the entire workspace of
    # 8 packages (0.225s per package).
    #
    # Current form:
    # $ hyperfine 'nix-instantiate -A ros.nova.workspace'
    #  Time (mean ± σ):       4.576 s ±  0.282 s    [User: 3.395 s, System: 0.492 s]
    #  Range (min … max):     4.113 s …  4.817 s    10 runs
    #
    # With all listed gitignore templates:
    # $ hyperfine 'nix-instantiate -A ros.nova.workspace'
    #  Time (mean ± σ):       5.380 s ±  0.403 s    [User: 4.095 s, System: 0.548 s]
    #  Range (min … max):     4.747 s …  6.238 s    10 runs
    #
    # Without added gitignore templates:
    # $ hyperfine 'nix-instantiate -A ros.nova.workspace'
    #   Time (mean ± σ):      2.764 s ±  0.151 s    [User: 1.649 s, System: 0.405 s]
    #   Range (min … max):    2.567 s …  2.941 s    10 runs
    #
    # Nop:
    # $ hyperfine 'nix-instantiate -A ros.nova.workspace'
    #   Time (mean ± σ):      2.749 s ±  0.093 s    [User: 1.606 s, System: 0.396 s]
    #   Range (min … max):    2.560 s …  2.862 s    10 runs
    nix-gitignore.gitignoreFilterSourcePure
      lib.cleanSourceFilter
      (
        [
          # Nix files in standard locations
          "default.nix"
          "shell.nix"
          "nix/"
        ]
        ++ builtins.foldl'
          # While gitignoreFilterSourcePure does support paths in the pattern
          # list, it is not possible to turn a derivation output path string
          # into a true path type.
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
          ])
        ++ lib.optional (builtins.pathExists (src + /.gitignore)) (src + /.gitignore)
        ++ extraPatterns
      )
      src;
}
