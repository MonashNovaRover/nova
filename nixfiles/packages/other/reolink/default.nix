{ lib
, python3Packages
}:
python3Packages.buildPythonApplication {
  pname = "reolink-ctl";
  version = "0.0.1";
  src = ./src;

  propagatedBuildInputs = with python3Packages; [
    onvif-zeep
    pynput
  ];

  format = "other";

  installPhase = ''
    mkdir -p $out/bin
    cp main.py $out/bin/reolink-ctl
    chmod +x $out/bin/reolink-ctl
  '';

  meta = with lib; {
    description = "Reolink PTZ camera control script";
    license = licenses.mit;
    platforms = platforms.linux;
  };
}
