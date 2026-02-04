{
  rosPackages = pkgs: with pkgs; {
    nova-bringup = callPackage ./nova_bringup { };
    nova-interfaces = callPackage ./nova_interfaces { };
    nova-rover-description = callPackage ./rover_description { };
    nova-controller-common = callPackage ./nova_controller_common { };

    ublox-dgnss-custom = callPackage ./nix/packages/ublox-dgnss { };

    # diff drive, pivot drive, strafe, 
  } // import ./arm { inherit pkgs; }
    // import ./auto { inherit pkgs; }
    // import ./chassis { inherit pkgs; }
    // import ./drive { inherit pkgs; }
    // import ./hardware_interfaces { inherit pkgs; }
    // import ./nova_generic { inherit pkgs; }
    // import ./old_inputs { inherit pkgs; }
    // import ./science { inherit pkgs; }
    // import ./simulations { inherit pkgs; }
    // import ./excavation_construction { inherit pkgs; };

  #pythonPackages = pythonPackages: with pythonPackages; {
    
  #};
}
