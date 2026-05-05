{ config, lib, ... }:

let
  cfg = config.nova.desktop.browser;
  chromiumExecBase = "nvidia-offload chromium --ozone-platform=x11 --use-angle=vulkan --enable-features=Vulkan,UseSkiaRenderer";
  chromiumExec = "${chromiumExecBase} %U";
  chromiumExecNewWindow = "${chromiumExecBase} --new-window %U";
  chromiumExecPrivate = "${chromiumExecBase} --incognito %U";
in
{
  options.nova.desktop.browser.enable = lib.mkEnableOption "Nova Rover user Chromium configuration";
  options.nova.desktop.browser.nvidiaOffload = lib.mkEnableOption "Enable NVIDIA offloaded Chromium desktop entry";

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
    ] ++ lib.optional cfg.nvidiaOffload "chromium-nvidia-offload.desktop";

    # Use nvidia offloading for chromium if enabled
    xdg.desktopEntries."chromium-nvidia-offload" = lib.mkIf cfg.nvidiaOffload {
      name = "Chromium (NVIDIA Offload)";
      exec = chromiumExec;
      startupNotify = true;
      terminal = false;
      icon = "chromium";
      type = "Application";
      categories = [ "Network" "WebBrowser" ];
      mimeType = [
        "application/pdf"
        "application/rdf+xml"
        "application/rss+xml"
        "application/xhtml+xml"
        "application/xhtml_xml"
        "application/xml"
        "image/gif"
        "image/jpeg"
        "image/png"
        "image/webp"
        "text/html"
        "text/xml"
        "x-scheme-handler/http"
        "x-scheme-handler/https"
        "x-scheme-handler/chromium"
      ];
      actions = {
        new-window = {
          name = "New Window";
          exec = chromiumExecNewWindow;
        };
        new-private-window = {
          name = "New Incognito Window";
          exec = chromiumExecPrivate;
        };
      };
    };
  };
}
