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
          linux_5_15 = super.linuxKernel.packages.linux_5_15.extend (linuxSelf: linuxSuper: {
            xone = linuxSuper.xone.overrideAttrs rec {
              version = "0.3-unstable-2024-12-23";
              src = self.fetchFromGitHub {
                owner = "dlundqvist";
                repo = "xone";
                # this is the version nixpkgs had before they updated xone and marked kernels older than 6.0 as broken.
                # I am unsure if it actually is broken for 5.15.
                rev = "6b9d59aed71f6de543c481c33df4705d4a590a31";
                hash = "sha256-MpxP2cb0KEPKaarjfX/yCbkxIFTwwEwVpTMhFcis+A4=";
              };
            };
          });
        };
      };
    })
  ];

  # Modern XBOX controllers
  hardware.xone.enable = true;
}
