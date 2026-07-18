{ pkgs }:

with pkgs;

{
  nova-gazebo = callPackage ./nova_gazebo { };
}
