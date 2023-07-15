# Hydra 

[Hydra](nixos.org/hydra) is a Nix-based continous build system. This repository
contains a [declarative project](https://github.com/NixOS/hydra/blob/master/doc/manual/src/plugins/declarative-projects.md)
for various CI and CD tasks, such as:

- Maintaining a [binary cache](./caches.md)
- Testing PRs (with status indicators on GitHub)
- Building ISO images of NixOS with our custom modules
- Building and serving documentation for our custom NixOS and Home Manager modules

## Using Hydra

Hydra can currently be accessed at https://hydra.ulna.leivenzon.id.au.
The username is `nova`, and the password is `***REMOVED***`.

To use the binary cache, see: [Binary caches](./caches.md).

## Setting up another Hydra instance

A warning in advance: Hydra is brittle and its documentation is practically
non-existant. The modules in this repository are designed to set up as much as
possible, but a number of things could still break with slightly different NixOS
configurations. Be prepared to crawl through source code and dead forum threads
to solve issues.

The manual can be found [here](https://nixos.org/hydra/manual).

1. Set up a NixOS server. Oracle Cloud with [NixOS-Infect](https://github.com/elitak/nixos-infect)
is recommended.
2. Add the [NixOS module](./nixos.md).
3. Configure the [`nova.ci.master` module](https://hydra.ulna.leivenzon.id.au/manual/nixos#novacimasterenable).
4. Log in to Hydra at _hydra.example.org_. The username is `nova` and the password is `***REMOVED***`. You'll need to log in twice - this is due to a layer
put in place to protect our binary cache.
5. Create a project with the following entries:  
**Identifier**: `nova`  
**Owner**: `nova`  
**Enable Dynamic RunCommand Hooks for Jobsets**: Yes  
**Declarative spec file**: `ci/spec.json`  
**Declarative input type**: Git checkout, `git@github.com:MonashNovaRover/nixfiles.git`  
6. Be patient - the first clone of Nixpkgs will take a while. Eventually, the CI
jobsets will be generated and begin executing.

> While Hydra does not display much progress information, some useful logging
can often be found in the journal entries of `hydra-evaluator.service` and
`hydra-queue-runner.service`.