{ fetchpatch
, firefoxPackages
, buildMozillaMach
, wrapFirefox
}:

let
  extraPatches = [
    # Allow creating RTCPeerConnections when the network is down
    # https://bugzilla.mozilla.org/show_bug.cgi?id=831926
    # This is needed for the GUI to make connections to the camera server.
    (fetchpatch {
      url = "https://github.com/hacker1024/gecko-dev/commit/542eb5d423c235cd0481cf4eee3f707c279a1304.patch";
      hash = "sha256-hzFmclT+q3dimrmTIaOQ1GHL04pJmrhVgz2ZNAwq8Uo=";
    })
  ];
in
(wrapFirefox
  ((firefoxPackages.override {
    buildMozillaMach = args: buildMozillaMach (args // {
      binaryName = "novafox";
      applicationName = "Novafox";
      extraPatches = args.extraPatches or [ ] ++ extraPatches;
    });
  }).firefox.override {
    enableOfficialBranding = false;
  })) {
  icon = "firefox";
  wmClass = "firefox";
}
