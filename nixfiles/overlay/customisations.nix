self: super:

{
  pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
    (
      pySelf: pySuper: {
        pynmeagps = pySuper.pynmeagps.overridePythonAttrs ({ ... }: {
          src = self.fetchFromGitHub {
            owner = "MonashNovaRover";
            repo = "pynmeagps";
            rev = "50e93bddeae0a2957363e18ee44a032307881675";
            hash = "sha256-SGTC/W7wv/we8Lo07geEA8h/PcsmG7BIzGHfgL3h4ZA=";
          };
        });
      }
    )
  ];
}
