self: super:

{
  # https://github.com/NixOS/nixpkgs/pull/225904#issuecomment-1692987472
  xsimd = super.xsimd.overrideAttrs ({ ... }: rec {
    version = "10.0.0";
    src = self.fetchFromGitHub {
      owner = "xtensor-stack";
      repo = "xsimd";
      rev = version;
      hash = "sha256-+ewKbce+rjNWQ0nQzm6O4xSwgzizSPpDPidkQYuoSTU=";
    };
  });
}
