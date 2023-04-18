{ pkgs ? import <nixpkgs> { } }:

pkgs.callPackage ./packages { }
