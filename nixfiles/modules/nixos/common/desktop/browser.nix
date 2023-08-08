{ config, lib, ... }:

let
  cfg = config.nova.desktop.browser;
  guiEnabled = config.nova.workspace.services.enable;
in
{
  options.nova.desktop.browser.enable = lib.mkEnableOption "Nova Rover Chromium configuration";

  config = lib.mkIf cfg.enable {
    programs.chromium = {
      enable = true;
      extraOpts = {
        # Startup
        RestoreOnStartup = if guiEnabled then 4 else 5;
        HomepageIsNewTabPage = false;
        HomepageLocation =
          if guiEnabled then
            "http://localhost"
          else
            "https://www.novarover.space";
        RestoreOnStartupURLs = lib.optional guiEnabled "http://localhost";

        # Appearance
        BrowserThemeColor = "#D4627A";
        ShowHomeButton = guiEnabled;
        NTPCardsVisible = false;
        NTPContentSuggestionsEnabled = false;
        NTPMiddleSlotAnnouncementVisible = false;

        # Shortcuts
        ManagedBookmarks = [
          {
            toplevel_name = "Bookmarks";
          }
          {
            name = "Monash Nova Rover";
            url = "https://www.novarover.space";
          }
        ] ++ lib.optionals guiEnabled [
          {
            name = "Rover GUI";
            url = "http://localhost";
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
        EditBookmarksEnabled = false;
        BrowserAddPersonEnabled = false;
        BookmarkBarEnabled = true;
        BrowserLabsEnabled = false;
        HideWebStoreIcon = true;
        NTPCustomBackgroundEnabled = false;
        UserAvatarCustomizationSelectorsEnabled = false;
      };
    };

    home-manager.nova.sharedModules = [{ nova.desktop.browser.enable = true; }];
  };
}
