{ 
  ament-cmake, 
  buildRosPackage, 
  fetchFromGitHub, 
  makeWrapper, 
  qt5, 
}:

buildRosPackage rec {
  pname = "ros2-unbag";
  version = "1.3.1";

  src = fetchFromGitHub {
    owner = "ika-rwth-aachen";
    repo = "ros2_unbag";
    rev = "v${version}";
    hash = "sha256-sgIBfwDoLqd/LVTWKns8oR51UkPg5n8sYJKP1LsyE4k=";
  };

  buildInputs = [ 
    qt5.qtbase 
    qt5.qtsvg 
  ];

  nativeBuildInputs = [ 
    ament-cmake 
    makeWrapper 
  ];

  # https://discourse.nixos.org/t/python-qt-qpa-plugin-could-not-find-xcb/8862
  postFixup = ''
    wrapProgram $out/lib/unbag/ros2_unbag_gui \
      --set QT_QPA_PLATFORM_PLUGIN_PATH \
      "${qt5.qtbase.bin}/lib/qt-${qt5.qtbase.version}/plugins/platforms"
  '';
}
