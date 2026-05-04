{ config, lib, ... }:

let
  cfg = config.nova.desktop.browser;
in
{
  options.nova.desktop.browser.enable = lib.mkEnableOption "Nova Rover user Chromium configuration";
  options.nova.desktop.browser.nvidiaOffload = lib.mkEnableOption "Use nvidia offloading for Chromium";

  config = lib.mkIf cfg.enable {
    programs.chromium = {
      enable = true;
      commandLineArgs = [
        "--profile-directory=nova"
      ];
      extensions = [
        "ddkjiahejlhfcafbddmgiahcphecmpfh" # ublock origin lite
      ];
    };

    dconf.settings."org/gnome/shell".favorite-apps = [
      "chromium-browser.desktop"
    ];

    # Use nvidia offloading for chromium if enabled
    xdg.desktopEntries.chromium-browser = lib.mkIf cfg.nvidiaOffload {
      name = "Chromium (Custom)";
      genericName = "Web Browser";
      exec = "nvidia-offload chromium --ozone-platform=x11 --use-angle=vulkan --enable-features=Vulkan,UseSkiaRenderer %U";
      icon = "chromium";
      terminal = false;
      categories = [ "Network" "WebBrowser" ];
      # mimeType = [ "text/html" "text/xml" "x-scheme-handler/http" "x-scheme-handler/https" ];
    };
  };
}
