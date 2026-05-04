{ config, lib, ... }:

let
  cfg = config.nova.desktop.browser;
in
{
  options.nova.desktop.browser.enable = lib.mkEnableOption "Nova Rover Chromium configuration";
  options.nova.desktop.browser.nvidiaOffload = lib.mkEnableOption "Use nvidia offloading for Chromium";

  config = lib.mkIf cfg.enable {
    programs.chromium = {
      enable = true;
      extraOpts = {
        # Startup
        RestoreOnStartup = 5;
        HomepageIsNewTabPage = false;
        HomepageLocation = "https://www.novarover.space";

        # Appearance
        BrowserThemeColor = "#D4627A";
        NTPCardsVisible = false;
        NTPContentSuggestionsEnabled = false;
        NTPMiddleSlotAnnouncementVisible = false;

        # Bookmarks
        ManagedBookmarks = [
          {
            name = "nova";
            url = "https://github.com/monashNovaRover/nova/";
          }
          {
            name = "GUI";
            url = "localhost:5173";
          }
        ];

        # Privacy and performance
        ClearBrowsingDataOnExitList = [
          "password_signin"
          "autofill"
        ];

        # Disable features that can't be managed declaratively
        DefaultBrowserSettingEnabled = false;
        BrowserSignin = 0;
        SyncDisabled = true;
        EditBookmarksEnabled = true;
        BrowserAddPersonEnabled = false;
        BookmarkBarEnabled = 0;
        BrowserLabsEnabled = false;
        HideWebStoreIcon = true;
        NTPCustomBackgroundEnabled = false;
        UserAvatarCustomizationSelectorsEnabled = false;
      };
    };

    home-manager.nova.sharedModules = [{ nova.desktop.browser.enable = true; nova.desktop.browser.nvidiaOffload = false; }];
  };
}
