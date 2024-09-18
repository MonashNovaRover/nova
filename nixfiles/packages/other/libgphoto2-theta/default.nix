{ libgphoto2
, fetchFromGitHub
}:

libgphoto2.overrideAttrs ({ pname, ... }: {
  pname = "${pname}-theta";
  src = fetchFromGitHub {
    owner = "codetricity";
    repo = "libgphoto2-theta";
    rev = "b41a4aa9d835df94eb170f3e4a4c46a57ff494c1";
    hash = "sha256-Guk19ROdUEc60WPxY/Gf+tdfRznTXHSRAiTBuYIZ410=";
  };
})
