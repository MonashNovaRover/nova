{ 
  buildRosPackage,
  fetchgit,
  ament-cmake,
  glm,
  glfw3,
  eigen,
  libpng,
  libjpeg,
  assimp,
}:

buildRosPackage {
  pname = "iridescence";
  version = "1.0.1";

  src = fetchgit {
    url = "https://github.com/koide3/iridescence";
    rev = "3200572daaf2ea11f660936890244d66efc727da";
    hash = "sha256-gQCRZAt+9NsIjZhnk+Fxnzp14Lsl3jhoBT4vsgiFv1g=";
  };

  patches = [
    ./patches/glfw.patch
  ];

  buildType = "ament_cmake";
  nativeBuildInputs = [ 
    ament-cmake 
  ];

  buildInputs = [
    glm
    glfw3
    eigen
    libpng
    libjpeg
    assimp
  ];

  cmakeFlags = [
    "-DGLFW_USE_WAYLAND=OFF"
  ];
}