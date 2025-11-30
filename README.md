<p align="center">
  <a href="https://www.novarover.space/">
    <picture>
      <source media="(prefers-color-scheme: dark)" srcset="https://images.squarespace-cdn.com/content/5d907deadfb23123fec64602/831da8ca-c17d-49cc-b325-0f7acbe62b2d/Monash+Nova+Rover+white+text+transparent.png?format=1500w&content-type=image%2Fpng">
      <img src="https://images.squarespace-cdn.com/content/5d907deadfb23123fec64602/831da8ca-c17d-49cc-b325-0f7acbe62b2d/Monash+Nova+Rover+white+text+transparent.png?format=1500w&content-type=image%2Fpng" width="500px" alt="Monash Nova Rover logo">
    </picture>
  </a>
</p>

<br/>

# Monash Nova Rover

Software for Monash Nova Rover's lunar and martian analogue rover.

`nova` is the set of packages that operate on-rover and in the base station for the [Monash Nova Rover](https://www.novarover.space/) student team. It is a collection of mostly ROS2 packages that we use and develop for the [ARCh](https://set.adelaide.edu.au/atcsr/australian-rover-challenge/) and [URC](https://urc.marssociety.org/) competitions. It is installed using the [Nix](https://nixos.org/) package manager, which is managed through [nixfiles](./nixfiles).

## Project Structure

The [`nova`](https://github.com/MonashNovaRover/nova) repository encapsulates:
- [`nixfiles`](./nixfiles)
  - Describes how the source code is built through nix.
- [`src`](./src)
  - Contains the code for all software for both on rover and base station.
  - [`rover`](./src/ros/rover)
    - All the source code for rover control and functionality.
  - [`nova-gui`](./src/ros/nova-gui)
    - A web based graphical user interface for the base station.
  - [`cameras2`](./src/ros/cameras2)
    - Camera discovery and streaming services between rover and base station.

```
├─ nixfiles               # Package manager
├─ src/                   # Source code
│  ├─ other               # Miscellaneous non-ros software
│  ├─ ros                 # The main folder containing all of our ROS2 software
│  │  ├─ cameras2         # Custom camera stack
│  │  ├─ nova-gui         # Graphical user interface for operators
│  │  ├─ rover            # Code for our autonomous system, robotic arm, drive and other payload controls.
```

## Development

Please follow [this notion guide](https://www.notion.so/Team-Software-Setup-Guide-8e94bf58c60c407195093c32814c6e7d) for a detailed guide if you're on the team.

In general:

1. Clone the repository
    ```shell
   git clone https://github.com/MonashNovaRover/nova.git
   ```
1. Build the nova workspace
    ```nix
    nix-build ./nixfiles -A pkgs.ros.nova-workspace
    ```
1. To run the different payloads and systems look in the relevant directory, or look at the [launch overview](./nixfiles/doc/rover-help.md).
    - There are also WIP launch scripts under `nixfiles/home/macros/launch` for more details, see the launch overview file above.

## Contributing

We loosely follow the [conventional commit standards](https://www.conventionalcommits.org/en/v1.0.0/#summary). In particular:

1. Create a branch following standard naming conventions such as `feat/`, `fix/`, etc.
2. Commit regularly
3. When you are ready to make a PR, rebase off of `master` first. Then make the PR.
4. Post the PR details in #software-pull-requests on Slack.
