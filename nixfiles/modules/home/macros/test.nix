{ pkgs ? import <nixpkgs> {}, route ? "" }:

let
  # We'll reuse the logic in both contexts
  startScript = ''
    if [ -z "$PTYXIS_STARTED" ]; then
      export PTYXIS_STARTED=1
      ptyxis --tab -x "bash -ic 'gui-shell --command \"gui-link; gui-run; exec bash\"; exec bash'" & \
      xdg-open "http://localhost:5173/${route}"
      exit 0
    fi
  '';

in pkgs.mkShell {
  # What happens when using nix-shell
  shellHook = startScript;

  # What happens when using nix-build
  buildCommand = ''
    mkdir -p $out/bin
    cat > $out/bin/start-gui <<'EOF'
    #!${pkgs.bash}/bin/bash
    ${startScript}
    EOF
    chmod +x $out/bin/start-gui
  '';
}
