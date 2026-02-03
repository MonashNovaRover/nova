{
  stdenv,
  lib,
  fetchFromGitHub,
  applyPatches,
  kernel,
  kernelModuleMakeFlags,
}:
stdenv.mkDerivation (finalAttrs: {
  pname = "xone";
  # newest version before linux 5.15 support was dropped
  # jetpack 6 for orin is linux 5.15.
  version = "0.4.12";

  src = applyPatches {
      src = fetchFromGitHub {
      owner = "dlundqvist";
      repo = "xone";
      tag = "v${finalAttrs.version}";
      hash = "sha256-Q/8f4CzlvqjV21yKkQQnr5GYM0YV5TgzVmeC/N6uuEU=";
    };
    patches = [
      ./for-loop-definitions.patch
    ];
  };

  setSourceRoot = ''
    export sourceRoot=$(pwd)/${finalAttrs.src.name}
  '';

  nativeBuildInputs = kernel.moduleBuildDependencies;

  makeFlags = kernelModuleMakeFlags ++ [
    "-C"
    "${kernel.dev}/lib/modules/${kernel.modDirVersion}/build"
    "M=$(sourceRoot)"
    "VERSION=${finalAttrs.version}"
  ];

  enableParallelBuilding = true;
  buildFlags = [ "modules" ];
  installFlags = [ "INSTALL_MOD_PATH=${placeholder "out"}" ];
  installTargets = [ "modules_install" ];

  meta = {
    description = "Linux kernel driver for Xbox One and Xbox Series X|S accessories";
    homepage = "https://github.com/dlundqvist/xone";
    license = lib.licenses.gpl2Plus;
    maintainers = with lib.maintainers; [
      rhysmdnz
      fazzi
    ];
    platforms = lib.platforms.linux;
  };
})
