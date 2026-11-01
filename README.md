![License](https://img.shields.io/github/license/julianxhokaxhiu/FF78Launcher) ![Overall Downloads](https://img.shields.io/github/downloads/julianxhokaxhiu/FF78Launcher/total?label=Overall%20Downloads) ![Latest Stable Downloads](https://img.shields.io/github/downloads/julianxhokaxhiu/FF78Launcher/latest/total?label=Latest%20Stable%20Downloads&sort=semver) ![Latest Canary Downloads](https://img.shields.io/github/downloads/julianxhokaxhiu/FF78Launcher/canary/total?label=Latest%20Canary%20Downloads) ![GitHub Actions Workflow Status](https://github.com/julianxhokaxhiu/FF78Launcher/actions/workflows/main-0.3.0.yml/badge.svg?branch=master)

# FF78Launcher

An alternative launcher for FF7/FF8 Steam and eStore editions.

## How to build

### Requirements (Windows only):

- [Visual Studio 2022](https://visualstudio.microsoft.com) (minimum) with the “Desktop development with C++” workload
- [CMake 3.25](https://cmake.org) or newer
- [Git](https://git-scm.com/install/windows)

### Steps (PowerShell or a Developer Command Prompt):

```
git clone --recurse-submodules https://github.com/julianxhokaxhiu/FF78Launcher.git
cd FF78Launcher

# vcpkg is a git submodule, so bootstrap it once
.\vcpkg\bootstrap-vcpkg.bat

cmake --preset Release
cmake --build --preset Release
```

## How to install

> **Please note:** Make a backup of your launcher or use the Steam Game files integrity check if something goes wrong and you're not able to launch the game.

1. Download the [latest release](https://github.com/ProjectXsent/FF78Launcher/releases/latest).
2. Extract it to your target FF7 or FF8 Steam folder.
3. Replace all files when asked.
4. You can now run the game via Steam as usual. Enjoy!

**REMEMBER:** Cloud sync is and always will be disabled. Please do not raise an issue for it, we are not going to implement it and we are not interested.

## How to change settings

After you install the launcher, you will find a file named [`FF78Launcher.toml`](misc/FF78Launcher.toml). Feel free to have a look at the file to know what can you change.

## Supported languages

For a full list of supported game launchers, see [this list](https://github.com/julianxhokaxhiu/FF78Launcher/blob/master/src/winmain.cpp#L23-L37)

## How to launch the Chocobo exe

If you want to launch the chocobo exe while using this custom launcher, please set the `launch_chocobo` flag to `true` in the [`FF78Launcher.toml`](misc/FF78Launcher.toml) config file. When you want to launch back the game, set back the `launch_chocobo` flag to `false`.

## Known issues

- ~Controller settings are not inherited from the official launcher~
