{ stdenv
, fetchFromGitHub
, fetchpatch
, cmake
, wrapQtAppsHook
, qtbase
, qtsvg
, elfutils
, ncurses
}:

stdenv.mkDerivation {
  pname = "groot";
  version = "1.0.0-unstable-2023-10-10"; # 1.0.0 was released in 2019, and many compilation fixes have been added since then.

  src = fetchFromGitHub {
    owner = "BehaviorTree";
    repo = "Groot";
    rev = "ca6c8f253f033bbdbe9294d1c9d0ac0beeb00241";
    fetchSubmodules = true;
    hash = "sha256-NFeCCtpiig4UQP5nlLOkl+aV4LZ0PTW0lWbo5Vc9hE8=";
  };

  patches = [
    # Link with ncurses instead of curses
    # https://github.com/BehaviorTree/Groot/pull/200
    (fetchpatch {
      url = "https://github.com/BehaviorTree/Groot/commit/9b1b796ef5fae6f0b7f939c8bdbd2b75a5757b4b.patch";
      hash = "sha256-8YTRHTm8mSs4B8q21n6Lr8c32P58sGGY7p0nKhyIKTU=";
    })
  ];

  nativeBuildInputs = [ cmake wrapQtAppsHook ];

  buildInputs = [ qtbase qtsvg elfutils ncurses ];
}
