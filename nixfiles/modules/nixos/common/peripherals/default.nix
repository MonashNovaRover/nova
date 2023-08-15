{ config, ... }:

{
  boot.extraModulePackages = with config.boot.kernelPackages; [
    # Modern XBOX controllers
    xone
  ];
}
