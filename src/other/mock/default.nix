{
  pythonPackages = pkgs: with pkgs; {
    mock-jcan = callPackage ./JCAN { };
  };
}

