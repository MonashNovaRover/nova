self: super:

{
  pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
    (
      pySelf: pySuper: {
        gphoto2 = pySuper.gphoto2.override {
          libgphoto2 = self.libgphoto2-theta;
        };
        json-tricks = pySuper.json-tricks.overridePythonAttrs {
          disabledTestPaths =
            [ "tests/test_pandas.py::test_pandas_series" ];
        };
      }
    )
  ];

  mavproxy = super.mavproxy.overridePythonAttrs {
    dontVersionCheck = true;
  };
}
