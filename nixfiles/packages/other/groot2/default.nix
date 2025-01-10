{ fetchurl
, appimageTools
}:

appimageTools.wrapType2 { 
  pname = "groot2";
  version = "1.6.1"; 

  src = fetchurl {
    url = "https://s3.us-west-1.amazonaws.com/download.behaviortree.dev/groot2_linux_installer/Groot2-v1.6.1-x86_64.AppImage";
    hash = "sha256-JUDtFKLFotTSzeXxtle15kUsLW6QoKf8BTmEY90ehUg=";
  };
}