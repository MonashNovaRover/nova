{ writeShellApplication
, nodePackages
, nova-gui
}:

writeShellApplication {
  name = "nova-gui-server";
  runtimeInputs = [ nodePackages.serve ];
  text = ''
    serve '${nova-gui}/share/nova-gui/www' \
      --single \
      --no-clipboard \
      --no-request-logging \
      "$@"
  '';
}