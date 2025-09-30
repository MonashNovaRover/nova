{
  # Some drivers need to be patched for certain kernels.
  nixpkgs.overlays = [
    (self: super: {
      linuxKernel = super.linuxKernel // {
        packages = super.linuxKernel.packages // {
          linux_5_10 = super.linuxKernel.packages.linux_5_10.extend (linuxSelf: linuxSuper: {
            xone = linuxSuper.xone.overrideAttrs rec {
              version = "0.3";
              src = self.fetchFromGitHub {
                owner = "medusalix";
                repo = "xone";
                rev = "v${version}";
                hash = "sha256-h+j4xCV9R6hp9trsv1NByh9m0UBafOz42ZuYUjclILE=";
              };
            };
          });
        };
      };
      linuxPackages = super.linuxPackages.extend (lpself: lpsuper: {
        xone = super.linuxPackages.xone.overrideAttrs (oldAttrs: {
          broken = false;
        });
      });
    })
  ];

  # Modern XBOX controllers
  hardware.xone.enable = true;
}
