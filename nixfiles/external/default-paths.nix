let
  srcDir = ../../src;

  # Searches for directories in a directory listing.
  filterDirectories = entries: builtins.foldl'
    (directories: entry:
      if entries.${entry} == "directory" || entries.${entry} == "symlink"
      then directories ++ [ entry ]
      else directories)
    [ ]
    (builtins.attrNames entries);

  # Find the categories (the first directory level in ./src)
  categories =
    if builtins.pathExists srcDir
    then filterDirectories (builtins.readDir srcDir)
    else [ ];

  # Look inside the category directories for repository directories.
  # Returns absolute paths.
  repoDirectories = builtins.foldl'
    (directories: category:
      directories
      ++ map
        (directory: srcDir + ("/" + category) + ("/" + directory))
        (filterDirectories (builtins.readDir (srcDir + ("/" + category)))))
    [ ]
    categories;
in
# Only return repositories that have a default.nix file.
builtins.filter
  (directory: builtins.pathExists (directory + "/default.nix"))
  repoDirectories
