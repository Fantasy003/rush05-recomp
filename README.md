# rush05-recomp

An unofficial, experimental static recompilation (hybrid) of the 2005 Nintendo DS game **Sonic Rush** for Windows x64, with an experimental Linux x64 build tested under Ubuntu WSL.

> [!IMPORTANT]
> **You need an original Sonic Rush USA Nintendo DS cartridge and an unmodified dump of your own copy.** The supported release has game code **ASCE**, revision **0**, size **64 MiB**, and SHA-1 `bc617530bb9b6e4179f2e54d7816c38676812dde`. Both the build script and the launcher check this hash. Keep your ROM private and only make and use a dump where permitted.

This repository contains the project source, runtime patches, and build tooling. The build generates its game code locally from your ROM. Game data, generated game code, save files, retail BIOS and firmware, and release executables are omitted from the repository.

## AI authorship

**ChatGPT by OpenAI made this recompilation build and its Windows integration**, guided by the project owner. ChatGPT assembled the build, wrote the launcher and Windows settings, investigated compatibility problems, and prepared this repository. The original game and the upstream recompiler, runtime, and reverse-engineering work were made by their respective authors.

## How it works

1. The build validates the ROM and extracts its ARM9 and ARM7 executable sections using the offsets in the DS cartridge header.
2. [ndsrecomp](https://github.com/RetroPortingToolKit/ndsrecomp) uses the separately supplied [rushrecomp](https://github.com/Tekkra-Pi/rushrecomp) analysis configs to locate ARM and Thumb code. It translates that code ahead of time to C, with dispatch tables that map guest program counters to the resulting host functions. The alignment guard rejects invalid ARM entry points that would otherwise round down and loop.
3. MinGW compiles the generated C and links the banks into the Windows runner. The runner provides the DS memory map, CPU scheduling, graphics, audio, touch input, and save devices. Guest code outside the static banks can use the runtime's instruction interpreter. The original ROM supplies game resources during play.
4. The runner boots directly with FreeBIOS and generated firmware. A runtime patch exposes the LCD HBlank status bit used by the original game's scanline polling code.
5. SDL3 presents the output and handles Windows input. **Focus** follows the active DS display in supported gameplay. **Fit-to-Screen** fills the output in that mode while keeping the dual-screen menu proportional. The 30/60/120 FPS settings control presentation. The 120 option adds blended frames on supported displays while the game's logic runs at DS timing.

This is a work in progress. The Windows build and startup path have been tested. Compatibility across the full campaign, special stages, and multiplayer is unverified. The shipped configuration has networking disabled.

## v28 settings and Linux build

In **PC Settings → Gameplay → Performance Governor**, choose On or Off and restart the game. The governor defaults to On. It can cap display transfer at 1080p and reduce internal 3D rendering to 720p under sustained load. Your selected resolution preset stays saved.

The Linux x64 build passed the performance governor, LCD routing, and PC settings tests under Ubuntu WSL. Linux desktop compatibility is still being tested. A graphical session is required to play. Extract the Linux package, then run `./play.sh "/path/to/Sonic Rush (USA).nds"` from the extracted directory. For an unpackaged build, run `./SonicRush` with the same ROM argument.

The Linux build tooling currently lives in the v28 workspace. It needs CMake, a C/C++ toolchain, Python 3, GNU patch, and a native SDL3 development installation. Set `SDL3_DIR` to the directory containing `SDL3Config.cmake` if CMake cannot find it.

## Build on Windows

Install **Git for Windows** and **Python 3.10 or later**. Extract [w64devkit x64 2.11.0](https://github.com/skeeto/w64devkit/releases/tag/v2.11.0) to a folder with a short path. It supplies the compiler and CMake used in this build.

Download the following ZIPs into one local folder using these filenames:

| Upstream ZIP | Filename |
| --- | --- |
| [ndsrecomp, pinned commit](https://github.com/RetroPortingToolKit/ndsrecomp/archive/f6f136774f43093cd9f60cf3794f82495fb3da2b.zip) | `ndsrecomp-f6f136774f43093cd9f60cf3794f82495fb3da2b.zip` |
| [arm-recomp-core, pinned commit](https://github.com/mstan/arm-recomp-core/archive/705e71b8aff3ad64fe78f77c3c94bd93158554dd.zip) | `arm-recomp-core-705e71b8aff3ad64fe78f77c3c94bd93158554dd.zip` |
| [recomp-ui, pinned commit](https://github.com/RetroPortingToolKit/recomp-ui/archive/01bff9477e6847b2281260f0a9526a85e45b8ba2.zip) | `recomp-ui-01bff9477e6847b2281260f0a9526a85e45b8ba2.zip` |
| [SDL3 3.4.20 MinGW development files](https://github.com/libsdl-org/SDL/releases/tag/release-3.4.20) | `SDL3-devel-3.4.20-mingw.zip` |

Separately obtain [toml++ 3.4.0's single header](https://raw.githubusercontent.com/marzer/tomlplusplus/v3.4.0/toml.hpp) and the ARM7/ARM9 configs in `ndsrecomp_title/configs` from the [pinned rushrecomp checkout](https://github.com/Tekkra-Pi/rushrecomp/tree/3068839d2291b019d7728100f6305765de1972f8). See [NOTICE.md](NOTICE.md) for the terms and provenance of those separate inputs.

Run from the repository root in PowerShell:

```powershell
python scripts/build_windows.py `
  --rom "C:\games\Sonic Rush (USA).nds" `
  --archives "C:\deps\archives" `
  --rush-config-dir "C:\deps\rushrecomp\ndsrecomp_title\configs" `
  --toml-header "C:\deps\toml.hpp" `
  --toolchain "C:\deps\w64devkit" `
  --jobs 2
```

The first build generates substantial C sources. Use a higher job count only if the machine has memory for it. The script writes to a fresh `build` directory. Pass `--work-dir` with a new path for another build.

Open `build/SonicRush/SonicRush.exe` and select the same ROM. It remembers your choice for the next launch. Hold **Shift** while launching or use `--choose-rom` to select it again. Press **Esc** to open the PC settings. Saves and preferences live under `%LOCALAPPDATA%\SonicRushRecomp`.

## Credits and license

- **Sonic Rush** was developed by Sonic Team and Dimps and published by SEGA for Nintendo DS. The original game supplies its graphics, audio, text, and levels at runtime. The game, characters, and marks belong to their respective rights holders. [Official game page](https://www.nintendo.com/en-gb/Games/Nintendo-DS/Sonic-Rush-272948.html)
- **Tekkra-Pi and rushrecomp contributors** provided the Sonic Rush ARM analysis and configuration used in the build. Their project documented the unaligned-entry-point issue. [rushrecomp](https://github.com/Tekkra-Pi/rushrecomp)
- **Matthew Stanley and RetroPortingToolKit contributors** provided the static recompiler, DS runtime, shared ARM core, and settings UI. [ndsrecomp](https://github.com/RetroPortingToolKit/ndsrecomp) · [arm-recomp-core](https://github.com/mstan/arm-recomp-core) · [recomp-ui](https://github.com/RetroPortingToolKit/recomp-ui)
- **melonDS contributors** provided the runtime's 3D device model and software rasterizer. **Gilead Kutnick** authored the DraStic FreeBIOS replacement used for direct boot. [melonDS](https://github.com/melonDS-emu/melonDS) · [FreeBIOS](https://github.com/mstan/freebios)
- **SDL contributors**, **Mark Gillard and toml++ contributors**, and **Chris Wellons and w64devkit contributors** provided display and input support, config parsing, and the Windows build toolchain. [SDL](https://github.com/libsdl-org/SDL) · [toml++](https://github.com/marzer/tomlplusplus) · [w64devkit](https://github.com/skeeto/w64devkit)

The original code and changes in **rush05-recomp** are offered under [GPL-3.0-or-later](LICENSE). Upstream components retain their own terms. This license grant covers project code and does not grant rights to the ROM, game assets, or trademarks. See [NOTICE.md](NOTICE.md) for the scope of the upstream licenses.
