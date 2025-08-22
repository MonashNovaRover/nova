<p align="center">
  <a href="https://www.novarover.space/">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://images.squarespace-cdn.com/content/5d907deadfb23123fec64602/831da8ca-c17d-49cc-b325-0f7acbe62b2d/Monash+Nova+Rover+white+text+transparent.png?format=1500w&content-type=image%2Fpng">
      <img src="https://images.squarespace-cdn.com/content/5d907deadfb23123fec64602/831da8ca-c17d-49cc-b325-0f7acbe62b2d/Monash+Nova+Rover+white+text+transparent.png?format=1500w&content-type=image%2Fpng" width="500px" alt="Monash Nova Rover logo">
    </picture>
  </a>
</p>

# Rover

[`rover`](https://github.com/MonashNovaRover/rover) is the set of packages that operate on-rover for the [Monash Nova Rover](https://www.novarover.space/) student team. It is a collection of mostly ROS2 packages that we use and develop for the [ARCh](https://set.adelaide.edu.au/atcsr/australian-rover-challenge/) and [URC](https://urc.marssociety.org/) competitions. It is installed using the [Nix](https://nixos.org/) package manager, which is managed through the [nixfiles](https://github.com/MonashNovaRover/nixfiles) repository.

# Getting Started 

* [nixfiles](https://github.com/MonashNovaRover/nixfiles) - make sure you follow the relevant steps to install Nix and `nixfiles` in your distro of choice.

# Contributing

1. Create a branch following standard naming conventions such as `feat/`, `fix/`, etc.
2. Commit regularly
3. When you are ready to make a PR, rebase off of `master` first. Then make the PR.
4. Post the PR details in #software-pull-requests on Slack.

# Other Project Repositories

Here are the other relevant repositories that we use with `rover`:

* [nixfiles](https://github.com/MonashNovaRover/nixfiles)
* [nova-gui](https://github.com/MonashNovaRover/nova-gui)
* [cameras2](https://github.com/MonashNovaRover/cameras2)
