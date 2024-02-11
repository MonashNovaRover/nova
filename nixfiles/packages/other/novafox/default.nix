{ lib
, fetchpatch
, firefoxPackages
, buildMozillaMach
, wrapFirefox
}@args:

# Hack to let .override apply to the wrapper itself.
{}:

let
  extraPatches = [
    # Allow creating RTCPeerConnections when the network is down
    # https://bugzilla.mozilla.org/show_bug.cgi?id=831926
    # This is needed for the GUI to make connections to the camera server.
    #
    # Verify these changes have applied by browsing to the modified JavaScipt
    # module: resource://gre/modules/media/PeerConnection.sys.mjs
    ./patches/webrtc-without-internet.patch
  ];
in
(wrapFirefox
  ((firefoxPackages.override {
    buildMozillaMach = args: buildMozillaMach (args // {
      applicationName = "Novafox";
      extraPatches = args.extraPatches or [ ] ++ extraPatches;
    });
  }).firefox.override {
    enableOfficialBranding = false;
  })) ({
  icon = "novafox";
  wmClass = "firefox";
})
