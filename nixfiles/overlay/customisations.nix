self: super:

{
  pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
    (
      pySelf: pySuper: {
        pynmeagps = pySuper.pynmeagps.overridePythonAttrs ({ prePatch ? "", ... }: {
          src = self.fetchFromGitHub {
            owner = "MonashNovaRover";
            repo = "pynmeagps";
            rev = "50e93bddeae0a2957363e18ee44a032307881675";
            hash = "sha256-SGTC/W7wv/we8Lo07geEA8h/PcsmG7BIzGHfgL3h4ZA=";
          };

          prePatch = prePatch + ''
            substituteInPlace pyproject.toml \
              --replace-warn '--cov --cov-report term-missing --cov-fail-under 95' '--cov --cov-report html --cov-fail-under 98'
          '';
        });
        gphoto2 = pySuper.gphoto2.override {
          libgphoto2 = self.libgphoto2-theta;
        };
      }
    )
  ];
}
