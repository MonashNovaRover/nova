{ config, ... }:

{
  boot.extraModulePackages = with config.boot.kernelPackages; [ tuxedo-keyboard ];
}
