{ fetchurl, fetchgit, linkFarm, runCommand, gnutar }: rec {
  offline_cache = linkFarm "offline" packages;
  packages = [
    {
      name = "keypress___keypress_0.2.1.tgz";
      path = fetchurl {
        name = "keypress___keypress_0.2.1.tgz";
        url  = "https://registry.yarnpkg.com/keypress/-/keypress-0.2.1.tgz";
        sha512 = "HjorDJFNhnM4SicvaUXac0X77NiskggxJdesG72+O5zBKpSqKFCrqmndKVqpu3pFqkla0St6uGk8Ju0sCurrmg==";
      };
    }
    {
      name = "onvif___onvif_0.8.0.tgz";
      path = fetchurl {
        name = "onvif___onvif_0.8.0.tgz";
        url  = "https://registry.yarnpkg.com/onvif/-/onvif-0.8.0.tgz";
        sha512 = "voGchPfdU2Ah+8vdTN8LjMUvzun25B0NCSTDdOCBm8ytEkyOuFeZZ4AWZHVSywehKPaHP2IKbc5IAFJDXnlE4w==";
      };
    }
    {
      name = "sax___sax_1.4.1.tgz";
      path = fetchurl {
        name = "sax___sax_1.4.1.tgz";
        url  = "https://registry.yarnpkg.com/sax/-/sax-1.4.1.tgz";
        sha512 = "+aWOz7yVScEGoKNd4PA10LZ8sk0A/z5+nXQG5giUO5rprX9jgYsTdov9qCchZiPIZezbZH+jRut8nPodFAX4Jg==";
      };
    }
    {
      name = "xml2js___xml2js_0.6.2.tgz";
      path = fetchurl {
        name = "xml2js___xml2js_0.6.2.tgz";
        url  = "https://registry.yarnpkg.com/xml2js/-/xml2js-0.6.2.tgz";
        sha512 = "T4rieHaC1EXcES0Kxxj4JWgaUQHDk+qwHcYOCFHfiwKz7tOVPLq7Hjq9dM1WCMhylqMEfP7hMcOIChvotiZegA==";
      };
    }
    {
      name = "xmlbuilder___xmlbuilder_11.0.1.tgz";
      path = fetchurl {
        name = "xmlbuilder___xmlbuilder_11.0.1.tgz";
        url  = "https://registry.yarnpkg.com/xmlbuilder/-/xmlbuilder-11.0.1.tgz";
        sha512 = "fDlsI/kFEx7gLvbecc0/ohLG50fugQp8ryHzMTuW9vSa1GJ0XYWKnhsUx7oie3G98+r56aTQIUB4kht42R3JvA==";
      };
    }
  ];
}
