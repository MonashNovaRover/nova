{ 
  buildRosPackage,
  fetchgit,
  ament-cmake,
  rclcpp, 
  std-msgs, 
  sensor-msgs, 
  std-srvs, 
  builtin-interfaces, 
  qt5, 
  yaml-cpp, 
  rcutils, 
  rcl, 
  rmw-implementation, 
  libstatistics-collector, 
  type-description-interfaces, 
  rcl-interfaces, 
  rcl-yaml-param-parser, 
  rosgraph-msgs, 
  statistics-msgs, 
  tracetools, 
  lttng-ust, 
  rcl-logging-interface, 
  geometry-msgs, 
  service-msgs, 
  rosidl-typesupport-fastrtps-c, 
  rosidl-typesupport-introspection-cpp, 
  rosidl-typesupport-introspection-c, 
  rosidl-typesupport-fastrtps-cpp, 
  rmw, 
  rosidl-dynamic-typesupport, 
  fastcdr, 
  rosidl-typesupport-cpp, 
  rosidl-typesupport-c, 
  rcpputils, 
  rosidl-runtime-c, 
  gccNGPackages_15, 
  gcc, 
  lib, 
  patchelf, 
  breakpointHook,
}:

buildRosPackage rec {
  pname = "dispatcher";
  version = "0.4.4";

  src = fetchgit {
    url = "https://github.com/nasa-jpl/dispatcher";
    rev = "v${version}";
    hash = "sha256-c8TFmNY1jXJ3whr12KcXR4Nc6/SwudGqmE6RAXf9ZHM=";
  };

  buildType = "ament_cmake";

  nativeBuildInputs = [ 
    ament-cmake 
    patchelf 
    breakpointHook 
  ];

  buildInputs = [
    rclcpp 
    std-msgs 
    sensor-msgs 
    std-srvs 
    builtin-interfaces 
    qt5.qtbase
    yaml-cpp 
  ];

  runtimeDependencies = [
    rclcpp
    sensor-msgs
    std-msgs
    std-srvs
    builtin-interfaces
    rcutils 
    rcl 
    rmw-implementation 
    libstatistics-collector 
    type-description-interfaces 
    rcl-interfaces 
    rcl-yaml-param-parser 
    rosgraph-msgs 
    statistics-msgs 
    tracetools 
    lttng-ust 
    rcl-logging-interface 
    geometry-msgs 
    service-msgs 
    rosidl-typesupport-fastrtps-c 
    rosidl-typesupport-introspection-cpp 
    rosidl-typesupport-introspection-c 
    rosidl-typesupport-fastrtps-cpp 
    rmw 
    rosidl-dynamic-typesupport 
    fastcdr 
    rosidl-typesupport-cpp
    rosidl-typesupport-c 
    rcpputils 
    rosidl-runtime-c
    qt5.qtbase
    yaml-cpp 
    gccNGPackages_15.libatomic 
    gccNGPackages_15.libstdcxx 
    gcc
  ];

  postInstall = ''
    patchelf --set-rpath "${lib.makeLibraryPath runtimeDependencies}" $out/lib/${pname}/${pname}

    # mkdir -p $out/bin
    # ln -s $out/lib/dispatcher/dispatcher $out/bin
  '';
}