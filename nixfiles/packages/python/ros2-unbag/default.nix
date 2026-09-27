{ buildPythonPackage
, fetchurl
, makeWrapper
, gtk3
, gtk4
, gsettings-desktop-schemas
, numpy
, opencv4
, pypcd4
, pyside6
, pyyaml
, tqdm
}:

buildPythonPackage {
  pname = "ros2-unbag";
  version = "1.2.3";
  format = "wheel";

  src = fetchurl {
    url = "https://files.pythonhosted.org/packages/f7/01/1800e410557207ef0ebacb128cb11ea3802afb175de6e3135f51c40adefa/ros2_unbag-1.2.3-py3-none-any.whl";
    hash = "sha256-J0P9XoMeF4hhX2n+jc0W7toH4xGg7aqDFyfzL3pdaAo=";
  };

  propagatedBuildInputs = [
    numpy
    opencv4
    pypcd4
    pyside6
    pyyaml
    tqdm
  ];

  nativeBuildInputs = [ makeWrapper ];

  postInstall = ''
    # Use explicit schema roots so standalone terminals do not depend on desktop session env.
    wrapProgram $out/bin/ros2-unbag \
      --prefix XDG_DATA_DIRS : "${gtk3}/share/gsettings-schemas/${gtk3.name}:${gtk4}/share/gsettings-schemas/${gtk4.name}:${gsettings-desktop-schemas}/share/gsettings-schemas/${gsettings-desktop-schemas.name}:${pyside6}/share" \
  '';

  pythonImportsCheck = [ "ros2_unbag" ];

  # The wheel pins exact dependency versions (numpy==1.26.4, pyside6==6.9.1, etc.)
  # that don't match the versions in this nixpkgs revision. The Nix-provided
  # versions are compatible at runtime; skip the strict check.
  dontCheckRuntimeDeps = true;
}
