# SPDX-License-Identifier: GPL-3.0-or-later
"""Build a local Windows runner from the user's verified Sonic Rush USA ROM."""

import argparse
import hashlib
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import struct
import subprocess
import sys
from zipfile import ZipFile


PROJECT = Path(__file__).resolve().parents[1]
ROM_SHA1 = "bc617530bb9b6e4179f2e54d7816c38676812dde"
TOML_SHA256 = "6b5172ad4dd6519aec67b919181fa7a38a2234131e5b2afa232dfe444819783e"
RUSH_CONFIG_HASHES = {
    7: "766b20409b7c4dbdac9cee8db90a473503b333521a47ff3ad89e16e6ad51959e",
    9: "75c8a6b964631557942f48fb06d92246150e170b2e9e821583ede0178e1bcaff",
}
ARCHIVES = {
    "nds": "ndsrecomp-f6f136774f43093cd9f60cf3794f82495fb3da2b.zip",
    "nds/external/arm-recomp-core": "arm-recomp-core-705e71b8aff3ad64fe78f77c3c94bd93158554dd.zip",
    "ui": "recomp-ui-01bff9477e6847b2281260f0a9526a85e45b8ba2.zip",
    "sdl": "SDL3-devel-3.4.20-mingw.zip",
}


def unpack_archive(archive, directory):
    directory.mkdir(parents=True, exist_ok=True)
    with ZipFile(archive) as bundle:
        for member in bundle.infolist():
            if member.is_dir():
                continue
            name = PurePosixPath(member.filename)
            parts = name.parts[1:]
            if name.is_absolute() or not parts or any(
                part == ".." or ":" in part or "\\" in part for part in name.parts
            ):
                raise ValueError(f"Unexpected dependency archive path: {name}")
            target = directory.joinpath(*parts)
            if not target.resolve().is_relative_to(directory.resolve()):
                raise ValueError(f"Archive member escapes its destination: {name}")
            target.parent.mkdir(parents=True, exist_ok=True)
            with bundle.open(member) as source, target.open("wb") as destination:
                shutil.copyfileobj(source, destination)


def apply_patches(nds, ui):
    for checkout, name in ((nds, "ndsrecomp-runner.patch"), (ui, "recomp-ui.patch")):
        subprocess.run(
            ["git", "apply", "--whitespace=nowarn", str(PROJECT / "patches" / name)],
            cwd=checkout, check=True,
        )
    # ARM state can dispatch only to word-aligned addresses. Filter invalid
    # roots before a function walk splits on them. See NOTICE for the report.
    finder = nds / "recompiler/finder/function_finder.cpp"
    source = finder.read_text(encoding="utf-8")
    anchor = "        if (!current_required && functions_.size() >= max_functions) {"
    if source.count(anchor) != 1:
        raise ValueError("Unsupported function finder source")
    guard = "        if (s.mode == CpuMode::Arm && (s.addr % 4u) != 0u) continue;\n"
    finder.write_text(source.replace(anchor, guard + anchor), encoding="utf-8")
    for source in (PROJECT / "src").iterdir():
        shutil.copyfile(source, nds / "runner/src" / source.name)
    runner = nds / "runner"
    shutil.copyfile(PROJECT / "cmake/Rush05Runner.cmake", runner / "Rush05Runner.cmake")
    with (runner / "CMakeLists.txt").open("a", encoding="utf-8") as cmake:
        cmake.write('\ninclude("${CMAKE_CURRENT_LIST_DIR}/Rush05Runner.cmake")\n')


def extract_cpu_inputs(rom, nds, images):
    for cpu, header_offset in ((9, 0x20), (7, 0x30)):
        offset, _, _, size = struct.unpack_from("<4I", rom, header_offset)
        if size == 0 or offset + size > len(rom):
            raise ValueError(f"ARM{cpu} ROM section is out of range")
        (images / f"arm{cpu}.bin").write_bytes(rom[offset:offset + size])
    embedded = (nds / "vendor/freebios/freebios_images.cpp").read_text(encoding="utf-8")
    for cpu in (9, 7):
        match = re.search(rf"nds_freebios{cpu}\[\d+\] = \{{(.*?)\}};", embedded, re.S)
        if match is None:
            raise ValueError(f"Could not locate the upstream FreeBIOS ARM{cpu} image")
        values = re.findall(r"0x([0-9a-fA-F]{2})", match.group(1))
        (images / f"freebios{cpu}.bin").write_bytes(bytes(int(value, 16) for value in values))


def build(options):
    if os.name != "nt":
        raise ValueError("This build script targets Windows x64")
    if options.jobs < 1:
        raise ValueError("--jobs must be positive")
    rom = options.rom.read_bytes()
    if len(rom) != 67108864 or hashlib.sha1(rom).hexdigest() != ROM_SHA1:
        raise ValueError("Provide your original, unmodified Sonic Rush USA Nintendo DS ROM (ASCE, rev 0)")

    configs = {}
    for cpu, expected in RUSH_CONFIG_HASHES.items():
        path = (options.rush_config_dir / f"arm{cpu}.toml").resolve()
        if hashlib.sha256(path.read_bytes().replace(b"\r\n", b"\n")).hexdigest() != expected:
            raise ValueError(f"Use the pinned upstream rushrecomp ARM{cpu} config listed in README.md")
        configs[cpu] = path
    if hashlib.sha256(options.toml_header.read_bytes()).hexdigest() != TOML_SHA256:
        raise ValueError("Use the unmodified toml++ 3.4.0 single header")
    for archive in ARCHIVES.values():
        if not (options.archives / archive).is_file():
            raise ValueError(f"Missing dependency archive: {archive}")
    compiler_path = options.toolchain.resolve() / "bin"
    cmake = compiler_path / "cmake.exe"
    if not cmake.is_file():
        raise ValueError("--toolchain must point to an extracted w64devkit x64 directory")
    if not shutil.which("git"):
        raise ValueError("Git for Windows must be available in PATH")
    env = os.environ.copy()
    env["PATH"] = str(compiler_path) + os.pathsep + env["PATH"]
    workspace = options.work_dir.resolve()
    workspace.mkdir(parents=True, exist_ok=False)
    for relative, filename in ARCHIVES.items():
        print(f"Staging {filename}", flush=True)
        unpack_archive(options.archives / filename, workspace / relative)
    nds = workspace / "nds"
    ui = workspace / "ui"
    sdl = workspace / "sdl/x86_64-w64-mingw32"
    apply_patches(nds, ui)
    images = workspace / "images"
    images.mkdir()
    extract_cpu_inputs(rom, nds, images)
    del rom

    cmake_options = [
        "-G", "MinGW Makefiles", "-DCMAKE_BUILD_TYPE=Release",
        "-DNDSRECOMP_COMPILER_CACHE=OFF", "-DCMAKE_POLICY_VERSION_MINIMUM=3.5",
        f"-DNDSRECOMP_TOMLPP_HEADER={options.toml_header.resolve()}",
    ]

    def run(*command):
        subprocess.run(command, check=True, env=env, cwd=workspace)

    def compile_target(directory, target):
        run(str(cmake), "--build", str(directory), "--target", target, "-j", str(options.jobs))

    recompiler_build = workspace / "recompiler-build"
    run(str(cmake), "-S", str(nds / "recompiler"), "-B", str(recompiler_build), *cmake_options)
    compile_target(recompiler_build, "nds_recompile")
    recompile = recompiler_build / "nds_recompile.exe"
    title_banks = workspace / "title-banks"
    title_banks.mkdir()
    freebios_banks = nds / "generated"
    freebios_banks.mkdir(exist_ok=True)
    for cpu in (9, 7):
        shards = 32 if cpu == 9 else 16
        title_bank = f"sonic_arm{cpu}_main"
        run(str(recompile), "--config", str(configs[cpu]), "--bin", str(images / f"arm{cpu}.bin"),
            "--out", str(title_banks), "--bank", title_bank, "--shards", str(shards))
        expected = [title_banks / f"{title_bank}_{index:02}.c" for index in range(shards)]
        expected.append(title_banks / f"{title_bank}_dispatch.c")
        if any(not path.is_file() for path in expected):
            raise ValueError(f"Recompiler failed to produce all of the ARM{cpu} title bank files")
        run(str(recompile), "--config", str(nds / f"bios/freebios{cpu}.toml"),
            "--bin", str(images / f"freebios{cpu}.bin"), "--out", str(freebios_banks),
            "--bank", f"freebios_arm{cpu}")
        # FreeBIOS direct boot needs no retail BIOS bank. The runtime linker
        # still expects these symbols, so supply an empty registry entry.
        (freebios_banks / f"arm{cpu}_bios.c").write_text(
            "/* This direct-boot build has no retail BIOS bank. */\n", encoding="utf-8")
        (freebios_banks / f"arm{cpu}_bios_dispatch.c").write_text(
            '#include "runtime_arm.h"\n'
            f'const NdsDispatchEntry g_dispatch_arm{cpu}_bios[1] = {{ {{0}} }};\n'
            f'const unsigned g_dispatch_arm{cpu}_bios_len = 0;\n', encoding="utf-8")

    runner_build = workspace / "runner-build"
    run(str(cmake), "-S", str(nds / "runner"), "-B", str(runner_build), *cmake_options,
        "-DNDS_BOOTSTRAP_FIRMWARE=ON", "-DNDS_ENABLE_COMPUTE_RENDERER=OFF",
        "-DNDS_SDL_BACKEND=SDL3", f"-DSDL3_DIR={sdl / 'lib/cmake/SDL3'}",
        f"-DNDS_RECOMP_UI_ROOT={ui}", f"-DNDS_TITLE_BANK_DIR={title_banks}",
        f"-DNDS_TITLE_ROM_SHA1={ROM_SHA1}", "-DNDS_TITLE_VERSION=rush05-recomp")
    compile_target(runner_build, "nds_runner")
    output = workspace / "SonicRush"
    output.mkdir()
    shutil.copyfile(runner_build / "SonicRush.exe", output / "SonicRush.exe")
    shutil.copyfile(sdl / "bin/SDL3.dll", output / "SDL3.dll")
    shutil.copyfile(PROJECT / "config/game.toml", output / "game.toml")
    run(str(output / "SonicRush.exe"), "--check-rom", str(options.rom.resolve()))
    print(f"Local build ready: {output / 'SonicRush.exe'}", flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--archives", type=Path, required=True)
    parser.add_argument("--rush-config-dir", type=Path, required=True)
    parser.add_argument("--toml-header", type=Path, required=True)
    parser.add_argument("--toolchain", type=Path, required=True)
    parser.add_argument("--work-dir", type=Path, default=PROJECT / "build")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    try:
        build(args)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"Build failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
