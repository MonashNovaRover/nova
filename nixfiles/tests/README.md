# NixOS Tests
3 levels of tests:
 - Module level tests:
   - https://nixos.org/manual/nixos/stable/#sec-writing-nixos-tests
   - These tests either bring up NixOS QEMU VMs or systemd-nspawn containers to run system level integration tests
   - Note that it is difficult to test graphical packages and nixos features with containers and VMs should be used instead
 - Package level tests:
   - https://github.com/NixOS/nixpkgs/blob/master/pkgs/README.md#package-tests
   - These tests can be used to check basic integration of a package
 - CheckPhase tests:
   - https://nixos.org/manual/nixpkgs/stable/#ssec-check-phase
   - These tests can be used to run unit tests that are included with the package


# What is in this directory?
There are currently two tests implemented in this directory both of which have been untouched since 2024...
Both tests must be run manually.

### `./cameras-webrtc`
This tested cameras2 with a demo GUI that used to be included with gstreamer-rs plugin to test webrtc
Unknown if this will work with cameras3. This test should be updated to work with cameras3

### `./networking`
A simple networking test to check the networking setup from 2024.
The setup is not what we have now and this test needs to be updated.