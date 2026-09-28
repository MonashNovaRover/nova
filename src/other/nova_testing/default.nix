{
  pythonPackages = pkgs: with pkgs; {
    mock-jcan = callPackage ./mock_jcan { };
    nova-pytest-framework = callPackage ./nova_pytest_framework { };
  };
}

