# Development Environment

This is a C++ project, built via CMake.
As it requires multiple dependencies that might not be trivial to build or install, we use [Nix][nix] to streamline the build process.

## Build

With Nix installed the following starts a build and displays build logs:

```
nix build -L
```

Implicitly, the `.#default` flake output is built, which builds d4.

## Development Shell

For manual builds and IDE setup, a Nix development shell can be used.
It configures the environment to pull in everything needed for a build.
To enter a development shell, use the following:

```
nix develop
```

### Manual Build

Inside the shell, a manual CMake build can be done.

To create and configure a build directory, use:

```
cmake -B build
```

For the actual build, use:

```
cmake --build build
```

### IDE

It is also possible to start an IDE from inside the shell such that it inherits the configured environment.
This should enable the IDE to configure and build the project.
For IDEs that can be started via CLI this should be as easy as issuing the following from inside the development shell:

```
<IDE command> .
```

## Cache

The CI uses a Nix cache which lives at [Cachix][cachix].
It can also be used locally so that Nix automatically uses cached builds when possible.
The Cachix cache has the name `` and can be enable for local (pull-only) usage using the following:

```
nix run nixpkgs#cachix use softvare-group
```

Depending on the Nix installation, this might require more configuration of Nix but it will usually output the steps needed.

## Dependencies

There are multiple dependencies which we build ourselves, they have their on build definitions at `nix` and a flake output each.
A manual build is usually not necessary as they automatically get built when required by Nix.

[cmake]: https://cmake.org
[nix]: https://nixos.org
[cachix]: https://www.cachix.org
