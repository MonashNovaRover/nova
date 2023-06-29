pkgs: lib:

let
  inherit (pkgs)
    nix-gitignore
    fetchFromGitHub;
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
        ++ (
          let
            gitignores = fetchFromGitHub {
              owner = "github";
              repo = "gitignore";
              rev = "4488915eec0b3a45b5c63ead28f286819c0917de";
              hash = "sha256-t/+ZQiGEziCqs8kIdlb/3/KBs0XQnHyQC+xoV2rzfbQ=";
            };
          in
          builtins.foldl'
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
              (gitignores + /C.gitignore)
              # (gitignores + /C++.gitignore) # Disabled to reduce evaluation time. Almost completely a subset of C.gitignore.
              (gitignores + /Python.gitignore)
              (gitignores + /CMake.gitignore)
              # (gitignores + /ROS.gitignore) # Faulty - filters out certain message files.
              (gitignores + /community/ROS2.gitignore)
              (gitignores + /community/Nix.gitignore)

              # Operating systems and editors
              (gitignores + /Global/Linux.gitignore)
              # (gitignores + /Global/macOS.gitignore) # Disabled to reduce evaluation time.
              (gitignores + /Global/Vim.gitignore)
              (gitignores + /Global/JetBrains.gitignore)
            ])
        )
        ++ lib.optional (builtins.pathExists (src + /.gitignore)) (src + /.gitignore)
        ++ extraPatterns
      )
      src;
}
