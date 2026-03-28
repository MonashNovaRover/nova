{ stdenv
, wine64
, fetchzip
}:
stdenv.mkDerivation {
  name = "livesplit-wine";
  src = fetchzip {
    url = "https://github.com/LiveSplit/LiveSplit/releases/download/1.8.37/LiveSplit_1.8.37.zip";
    hash = "sha256-iP2zJGZsrVAKSz150zMukBUzq8gKDWgLsghYy9QyD98=";
    stripRoot = false;
  };

  installPhase = ''
    runHook preInstall

    mkdir -p $out/bin
    mkdir -p $out/opt/livesplit
    cp -r * $out/opt/livesplit/
    echo "#!/usr/bin/env bash" >> $out/bin/livesplit
    echo "exec ${wine64}/bin/wine64 $out/opt/livesplit/livesplit.exe $@" >> $out/bin/livesplit

    chmod +x $out/bin/livesplit

    runHook postInstall
  '';
}

