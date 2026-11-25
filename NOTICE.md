# Provenance and licensing

The original source and build tooling in this repository, including project additions to the patches, are offered under the GNU General Public License, version 3 or (at your option) any later version. See [LICENSE](LICENSE). ChatGPT prepared these contributions at the project owner's direction.

## Patch context and separately supplied components

| Component | Use and license |
| --- | --- |
| [ndsrecomp at f6f1367](https://github.com/RetroPortingToolKit/ndsrecomp/tree/f6f136774f43093cd9f60cf3794f82495fb3da2b) | The build uses its recompiler under the upstream **PolyForm Noncommercial 1.0.0** terms. The runtime patch touches files in `runner/src`. Upstream identifies most of that tree as **MIT**, with its melonDS-derived frontend and related code under **GPL-3.0-or-later**. Read the upstream [LICENSE](https://github.com/RetroPortingToolKit/ndsrecomp/blob/f6f136774f43093cd9f60cf3794f82495fb3da2b/LICENSE) and [THIRD_PARTY_ATTRIBUTION.md](https://github.com/RetroPortingToolKit/ndsrecomp/blob/f6f136774f43093cd9f60cf3794f82495fb3da2b/THIRD_PARTY_ATTRIBUTION.md) for exact scope. |
| [recomp-ui at 01bff94](https://github.com/RetroPortingToolKit/recomp-ui/tree/01bff9477e6847b2281260f0a9526a85e45b8ba2) | Settings UI library. Upstream **MIT** patch context. |
| [arm-recomp-core at 705e71b](https://github.com/mstan/arm-recomp-core/tree/705e71b8aff3ad64fe78f77c3c94bd93158554dd) | Shared ARM decoder, code generation and interpreter support. **MIT**. |
| [melonDS](https://github.com/melonDS-emu/melonDS) | The ndsrecomp runner links the upstream vendored GPU model and software renderer. **GPL-3.0-or-later**. |
| [FreeBIOS](https://github.com/mstan/freebios) | Gilead Kutnick's open replacement BIOS, as embedded in the pinned ndsrecomp dependency. **BSD-2-Clause**. |
| [SDL3](https://github.com/libsdl-org/SDL) and [toml++](https://github.com/marzer/tomlplusplus) | SDL3 uses the **zlib** license and toml++ uses **MIT**. Obtain them separately. |
| [Tekkra-Pi/rushrecomp at 3068839](https://github.com/Tekkra-Pi/rushrecomp/tree/3068839d2291b019d7728100f6305765de1972f8) | Independently supplied ARM7 and ARM9 analysis configs. The pinned repository does not show a license grant for copying those files into this project, so neither its configs nor its patch are distributed here. Its report of an unaligned ARM root motivated the alignment check implemented in our build script. |

The [MIT notices](licenses/) retain upstream authorship for the patch context. The public project license does not relicense the separate compiler, dependencies, or their files. A native runner that links the vendored melonDS code is subject to the GPL's rules for combined works. This repository provides a local build instead of a binary distribution.

## Game and artwork

The executable reads the game and its assets from the user's own ROM. The public tree omits those assets, ROM dumps, generated game banks, and BIOS or firmware dumps. The logo supplied for the earlier private Windows package is also omitted. SEGA and the respective owners retain their rights to the game, characters, and marks. This project is independent of SEGA, Nintendo, Sonic Team, and Dimps.
