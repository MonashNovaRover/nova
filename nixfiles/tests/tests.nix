{ runTest }:

{
  cameras-webrtc = runTest ./cameras-webrtc;
  networking = runTest ./networking;
  can = runTest ./can;
}
