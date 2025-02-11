# Environment setup

_Note: this setup guide is currently for Windows exclusively_

## Bootstrapping

For initializing submodules & setup vcpkg, a package manager for our project to use.

```bash
bootstrap.bat
```

## Env setup

### Cmake

- Download the newest installer from [here](https://cmake.org/download/)

### Ninja

- Download the newest pre-compiled version of ninja from [here](https://github.com/ninja-build/ninja/releases)

- create a folder named `Ninja` under `C:\Program Files\`

- copy `ninja.exe` under `C:\Program Files\Ninja`

- add `C:\Program Files\Ninja` to PATH

### Ccache

- Download the latest pre-compiled version of ccache from [here](https://github.com/ccache/ccache/releases/tag/v4.10.2)

- Copy the entire folder under `C:\Program Files\`

- add `C:\Program Files\ccache-your.version.here-windows-x86_64` to PATH

## Build the project

```bash
build.bat
```
