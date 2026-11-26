// Windows entry point for the Sonic Rush ndsrecomp build.
// SPDX-License-Identifier: GPL-3.0-or-later
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include "sha1.h"
#include "sonic_pc_settings.h"

int nds_runner_main(int argc, char** argv);
namespace fs = std::filesystem;
static constexpr const char* kRomSha1 = "bc617530bb9b6e4179f2e54d7816c38676812dde";

static std::string local_path(const fs::path& path) {
    // Preserve filenames containing characters outside the active code page
    // when the volume provides DOS aliases; the upstream runner uses narrow IO.
    std::wstring value = path.wstring();
    DWORD size = GetShortPathNameW(value.c_str(), nullptr, 0);
    if (size) {
        std::wstring short_path(size, L'\0');
        DWORD written = GetShortPathNameW(value.c_str(), short_path.data(), size);
        if (written && written < size) {
            short_path.resize(written);
            value = std::move(short_path);
        }
    }
    BOOL lossy = FALSE;
    const bool utf8 = GetACP() == CP_UTF8;
    const DWORD flags = utf8 ? 0 : WC_NO_BEST_FIT_CHARS;
    int n = WideCharToMultiByte(CP_ACP, flags, value.c_str(),
                               -1, nullptr, 0, nullptr, utf8 ? nullptr : &lossy);
    if (!n || lossy) throw std::runtime_error("La ruta contiene caracteres no admitidos. Move el juego a una carpeta con un nombre simple.");
    std::string result(n, '\0');
    WideCharToMultiByte(CP_ACP, flags, value.c_str(), -1,
                        result.data(), n, nullptr, nullptr);
    result.pop_back();
    return result;
}

static fs::path choose_rom() {
    wchar_t file[32768] = {};
    OPENFILENAMEW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.lpstrFilter = L"Sonic Rush para Nintendo DS (*.nds)\0*.nds\0";
    dialog.lpstrFile = file;
    dialog.nMaxFile = static_cast<DWORD>(std::size(file));
    dialog.lpstrTitle = L"Sonic Rush - Seleccionar el juego";
    dialog.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR | OFN_EXPLORER;
    if (GetOpenFileNameW(&dialog)) return file;
    if (CommDlgExtendedError()) throw std::runtime_error("No se pudo abrir el selector de archivos.");
    return {};
}

static bool validate_rom(const fs::path& path) {
    std::error_code error;
    if (fs::file_size(path, error) != 67108864 || error) return false;
    std::ifstream input(path, std::ios::binary);
    std::vector<unsigned char> data(67108864);
    if (!input.read(reinterpret_cast<char*>(data.data()), data.size())) return false;
    return gba::sha1(data.data(), data.size()).hex() == kRomSha1;
}

static fs::path launcher_data_path() {
    wchar_t local_appdata[32768];
    const DWORD count = GetEnvironmentVariableW(L"LOCALAPPDATA", local_appdata,
                                                std::size(local_appdata));
    if (!count || count >= std::size(local_appdata))
        throw std::runtime_error("No se pudo ubicar la carpeta para guardar partidas.");
    return fs::path(local_appdata) / L"SonicRushRecomp";
}

static fs::path read_rom_path(const fs::path& config_file) {
    constexpr std::uintmax_t max_path_bytes = 4 * 32768;
    std::error_code error;
    const auto size = fs::file_size(config_file, error);
    if (error || size == 0 || size > max_path_bytes) return {};
    std::u8string utf8(static_cast<size_t>(size), u8'\0');
    std::ifstream input(config_file, std::ios::binary);
    if (!input.read(reinterpret_cast<char*>(utf8.data()), utf8.size()) ||
        utf8.find(u8'\0') != std::u8string::npos) return {};
    try {
        fs::path path(utf8);
        return path.is_absolute() ? path : fs::path{};
    } catch (const fs::filesystem_error&) {
        // A corrupt or incomplete settings file is treated as no selection.
        return {};
    }
}

static void remember_rom_path(const fs::path& config_file, const fs::path& rom) {
    fs::path pending = config_file;
    pending += L"." + std::to_wstring(GetCurrentProcessId()) + L".tmp";
    const auto utf8 = rom.u8string();
    std::ofstream output(pending, std::ios::binary | std::ios::trunc);
    output.write(reinterpret_cast<const char*>(utf8.data()), utf8.size());
    output.close();
    if (output && MoveFileExW(pending.c_str(), config_file.c_str(),
                             MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return;
    std::error_code error;
    fs::remove(pending, error);
    std::fprintf(stderr, "[launcher] Could not remember the ROM path.\n");
}

int main(int argc, char** argv) {
    // Keep the original CLI available for diagnostics without showing a picker.
    if (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "--runtime")) {
        if (std::string(argv[1]) == "--runtime") return nds_runner_main(argc-1, argv+1);
        return nds_runner_main(argc, argv);
    }
    const bool check_only = argc > 1 && std::string(argv[1]) == "--check-rom";
    try {
        int wide_count = 0;
        std::unique_ptr<LPWSTR, decltype(&LocalFree)> wide_args(
            CommandLineToArgvW(GetCommandLineW(), &wide_count), &LocalFree);
        if (!wide_args) throw std::runtime_error("No se pudieron leer los argumentos.");
        fs::path rom;
        std::vector<std::string> battle_arguments;
        const std::wstring_view option = wide_count > 1 ? wide_args.get()[1] : L"";
        const bool battle = option == L"--battle-host" || option == L"--battle-join";
        const auto set_battle_arguments = [&](int role, const std::string& local_ip,
                                               const std::string& peer_ip) {
            battle_arguments = {"--instance-index", role == 1 ? "0" : "1",
                "--local-wireless", "on", "--local-wireless-bind", local_ip,
                "--local-wireless-peer", peer_ip};
        };
        const bool choose_only = wide_count == 2 &&
            option == L"--choose-rom";
        if (battle) {
            if (wide_count != 4)
                throw std::runtime_error("Usage: SonicRush.exe --battle-host|--battle-join YOUR_IPV4 FRIEND_IPV4");
            std::string ip[2];
            for (int i = 0; i < 2; ++i) {
                for (wchar_t c : std::wstring_view(wide_args.get()[i + 2])) {
                    if ((c < L'0' || c > L'9') && c != L'.')
                        throw std::runtime_error("Enter both IPv4 addresses as four numbers separated by dots.");
                    ip[i].push_back(static_cast<char>(c));
                }
            }
            if (!sonic_pc_battle_ipv4(ip[0]) || !sonic_pc_battle_ipv4(ip[1]))
                throw std::runtime_error("Enter a valid IPv4 address for each PC.");
            set_battle_arguments(option == L"--battle-host" ? 1 : 2, ip[0], ip[1]);
        } else if (check_only && wide_count == 3) rom = wide_args.get()[2];
        else if (!check_only && wide_count == 2 && !choose_only) rom = wide_args.get()[1];
        else if ((check_only && wide_count != 3) ||
                 (!check_only && wide_count != 1 && !choose_only)) {
            std::fprintf(stderr, "Uso: SonicRush.exe [juego.nds] | --choose-rom | --check-rom juego.nds\n");
            return 2;
        }

        if (check_only && rom.empty()) return 2;
        const fs::path user_directory = check_only ? fs::path{} : launcher_data_path();
        const fs::path rom_config = user_directory / L"rom-path.txt";
        const bool bypass_recent = choose_only || (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool used_remembered_path = false;
        if (rom.empty() && !check_only && !bypass_recent) {
            auto remembered = read_rom_path(rom_config);
            if (!remembered.empty() && validate_rom(remembered)) {
                rom = std::move(remembered);
                used_remembered_path = true;
            }
        }
        if (rom.empty()) rom = choose_rom();
        if (rom.empty()) return 0;
        rom = fs::absolute(rom);
        if (!used_remembered_path && !validate_rom(rom)) {
            const char* message = "El archivo no coincide con Sonic Rush (USA) compatible con esta version. Selecciona el archivo .nds original de 64 MB, sin parches.";
            std::fprintf(stderr, "%s\n", message);
            if (!check_only) MessageBoxA(nullptr, message, "Sonic Rush", MB_OK | MB_ICONERROR);
            return 2;
        }
        if (check_only) {
            std::printf("ROM compatible: ASCE, SHA-1 %s\n", kRomSha1);
            return 0;
        }

        wchar_t executable[32768];
        DWORD length = GetModuleFileNameW(nullptr, executable, std::size(executable));
        if (!length || length >= std::size(executable)) throw std::runtime_error("No se pudo ubicar el programa.");
        fs::path app_directory = fs::path(executable).parent_path();
        fs::create_directories(user_directory);
        // Store user data outside the installation and beside neither the ROM
        // nor the executable, so read-only locations remain usable.
        fs::path log = user_directory / L"SonicRush.log";
        if (!_wfreopen(log.c_str(), L"w", stderr)) throw std::runtime_error("No se pudo crear el registro de ejecucion.");
        _wfreopen(log.c_str(), L"a", stdout);
        if (!SetCurrentDirectoryW(user_directory.c_str())) throw std::runtime_error("No se pudo abrir la carpeta de partidas.");
        if (!std::getenv("NDS_HOSTPROF")) _putenv_s("NDS_HOSTPROF", "off");
        const std::string runtime_settings = local_path(user_directory) + "\\settings.toml";
        if (!battle) {
            sonic_pc_load(sonic_pc_settings_path(runtime_settings));
            if (g_sonic_pc.battle_role && !g_sonic_pc.battle_local_ip.empty() &&
                !g_sonic_pc.battle_peer_ip.empty()) {
                set_battle_arguments(g_sonic_pc.battle_role, g_sonic_pc.battle_local_ip,
                                     g_sonic_pc.battle_peer_ip);
            }
        }
        std::vector<std::string> arguments = {
            "SonicRush.exe", "--interactive", "--rom", local_path(rom),
            "--freebios", "--generated-firmware", "--boot", "direct",
            "--config", local_path(app_directory / L"game.toml"),
            "--save-path", local_path(user_directory) + "\\sonic-rush.sav",
            "--runtime-settings-path", runtime_settings,
            "--savestate-dir", local_path(user_directory / L"states"),
            "--no-coverage-manifest", "--network", "off", "--diagnostics", "off"
        };
        arguments.insert(arguments.end(), battle_arguments.begin(), battle_arguments.end());
        if (!used_remembered_path) remember_rom_path(rom_config, rom);
        std::fprintf(stderr, "[launcher] ROM path: %s\n",
                     used_remembered_path ? "remembered selection" : "new selection");
        std::vector<char*> pointers;
        for (auto& argument : arguments) pointers.push_back(argument.data());
        int result = nds_runner_main(static_cast<int>(pointers.size()), pointers.data());
        if (result != 0) MessageBoxW(nullptr,
            L"El juego no pudo continuar. Podes consultar el registro en %LOCALAPPDATA%\\SonicRushRecomp\\SonicRush.log.",
            L"Sonic Rush", MB_OK | MB_ICONERROR);
        return result;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        if (!check_only) MessageBoxA(nullptr, e.what(), "Sonic Rush", MB_OK | MB_ICONERROR);
        return 1;
    }
}
