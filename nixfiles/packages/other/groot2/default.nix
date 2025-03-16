{ fetchurl
, appimageTools
}:

appimageTools.wrapType2 { 
  pname = "groot2";
  version = "1.6.1"; 

  src = fetchurl {
    # This URL can be taken down at any time without warning. The AppImage could be downloaded and stored in the monorepo to 
    # ward against this, but would take up ~109.7 MB, so we'll avoid it unless the URL does go down. Currently, the AppImage
    # is downloaded on Victor's laptop.
    url = "https://s3.us-west-1.amazonaws.com/download.behaviortree.dev/groot2_linux_installer/Groot2-v1.6.1-x86_64.AppImage";
    hash = "sha256-JUDtFKLFotTSzeXxtle15kUsLW6QoKf8BTmEY90ehUg=";
  };
}