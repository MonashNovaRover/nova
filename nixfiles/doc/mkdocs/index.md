# Overview

This manual is  and Home Manager module trees.

Use the subpages to browse the option sets that are currently documented:

- Home Manager - generated from `nixfiles/modules/home`
- NixOS - generated from `nixfiles/modules/nixos`

To add another documentation section later, extend the `sections` list in this
file with another entry.

Jetson support depends on the external `jetpack-nixos` source. To keep this
documentation build offline and restricted-mode safe, that external module is
not imported here, so Jetson-specific upstream options are omitted from the
generated option list.