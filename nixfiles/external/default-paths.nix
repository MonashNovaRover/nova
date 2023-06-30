let
  # Searches for directories in a directory listing.
  filterDirectories = entries: builtins.foldl'
    (directories: entry:
      if entries.${entry} == "directory"
      then directories ++ [ entry ]
      else directories)
    [ ]
    (builtins.attrNames entries);

  # Find the categories (the first directory level in ./src)
  categories = filterDirectories (builtins.readDir ./src);

  # Look inside the category directories for repository directories.
  # Returns absolute paths.
  repoDirectories = builtins.foldl'
    (directories: category:
      directories
      ++ map
        (directory: ./src + (/. + category) + (/. + directory))
        (filterDirectories (builtins.readDir (./src + (/. + category)))))
    [ ]
    categories;
in
# Only return repositories that have a default.nix file.
builtins.filter
  (directory: builtins.pathExists (directory + /default.nix))
  repoDirectories
