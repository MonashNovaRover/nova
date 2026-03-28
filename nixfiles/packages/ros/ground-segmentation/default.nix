{ 
  buildRosPackage,
  fetchgit,
  cmake,
  eigen,
  pcl,
  boost,
  glog,
  nanoflann,
  gtest,
  std-srvs
}:

buildRosPackage {
  pname = "ground-segmentation";
  version = "0.0.0";

  src = fetchgit {
    url = "https://github.com/dfki-ric/ground_segmentation";
    rev = "f9997da623c0b0ee43b70187331c8f6d242915bc"; 
    hash = "sha256-3zKXSvQKWP6NdsLkvh4kZjxPySJwR52f0Did00Dah6A="; 
  };

  buildType = "cmake";

  nativeBuildInputs = [ 
    cmake 
  ];

  buildInputs = [
    eigen
    pcl
    boost
    glog
    nanoflann
    gtest
    std-srvs
  ];
}