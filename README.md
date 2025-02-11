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

## Guidelines for development using vscode

### Recommended extensions

- `command-runner`: to run customized commands from command line

  In `.vscode/settings.json`, you may want to setup some build commands as shortcuts:

  ```plaintext
    "command-runner.commands": {
    "build windows release": "build.bat",
    "build windows debug": "build.bat --debug"
  }
  ```

- `C/C++`: only for debugging, disable the IntelliSense feature

- `clangd`: C/C++ completion, navigation, and insights, faster than the `C/C++` extension, especially in larger codebases

  Here's a typical configuration you may want to use inside `.vscode/settings.json`

  ```plaintext
  "clangd.arguments": [
  // number of async workers used by clangd. background index also uses this many workers
  "-j=8",

  // specify a path to look for compile_commands.json
  "--compile-commands-dir=.vscode/",

  // never insert headers automatically
  "--header-insertion=never",

  // load pre compiled headers from memory, this is faster than loading from disk
  "--pch-storage=memory",

  // enable clang tidy
  "--clang-tidy"
  ],
  ```

- `Clang-Format`: code formatting

## Learning resources

[Daxa Tutorial](https://tutorial.daxa.dev/)

[Vulkan Tutorial](https://vulkan-tutorial.com/)
