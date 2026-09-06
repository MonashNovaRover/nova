self: super:

{
  pythonPackagesExtensions = super.pythonPackagesExtensions ++ [
    (
      pySelf: pySuper: {
        gphoto2 = pySuper.gphoto2.override {
          libgphoto2 = self.libgphoto2-theta;
        };
        # json-tricks = pySuper.json-tricks.overridePythonAttrs (old: {
        #   doCheck = false;
        # });
      }
    )
  ];

  mavproxy = super.mavproxy.overridePythonAttrs (old: {
    dontVersionCheck = true;
  });
}
