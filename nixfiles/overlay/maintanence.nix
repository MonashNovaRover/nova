self: super:

{
  pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
    (pyself: pysuper: {
      argcomplete = pysuper.argcomplete.overridePythonAttrs ({ pname, ... }: rec {
        version = "3.1.1";
        src = pyself.fetchPypi {
          inherit pname version;
          hash = "sha256-bExWPxTwFECq/6Pq4TRBxdsjV7Xuxjmr58CxUzRiff8=";
        };
      });
    })
  ];
}
