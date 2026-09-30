/*
 * Copyright (c) 2026 Llucs
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <iostream>
#include <print>
#include <string>
#include <vector>
#include <filesystem>
#include <cctype>
#include <chrono>
#include <thread>
#include "muninn/muninn.h"

static const char* tool_name() {
    return "MUNINN";
}

static void print_usage() {
    std::println("Usage: {} [options]", tool_name());
    std::println("{} — memory's raven. Samsung firmware flashing tool for Linux. Version: {}", tool_name(), muninn_get_version());
    std::println("");
    std::println("Options:");
    std::println("  -h                  Show this help message");
    std::println("  -v                  Show version");
    std::println("  -w                  Show license");
    std::println("  -l                  List detected Download Mode devices");
    std::println("  -d <path>            Select a specific USB device path (e.g. /dev/bus/usb/001/002)");
    std::println("  -b <file>            Bootloader archive (.tar or .tar.md5)");
    std::println("  -a <file>            AP archive (.tar or .tar.md5)");
    std::println("  -c <file>            CP archive (.tar or .tar.md5)");
    std::println("  -s <file>            CSC archive (.tar or .tar.md5)");
    std::println("  -u <file>            UMS archive (.tar or .tar.md5)");
    std::println("  --check-only         Validate PIT + archives and exit without flashing");
    std::println("  --allow-unknown      Allow archive entries without a PIT match (disabled by default)");
    std::println("  --reboot             Reboot device after flashing");
    std::println("  --no-reboot          Never reboot device after flashing (overrides --reboot)");
    std::println("  --redownload         Reboot into download mode if supported");
    std::println("  --efs-clear          Clear EFS partition during flash (repair IMEI/calibration)");
    std::println("  --bl-update          Signal bootloader update to device");
    std::println("  --wait-device <secs>  Wait up to N seconds for a Download Mode device before proceeding");
    std::println("  --json               Machine-readable output (device list / flash summary)");
    std::println("");
    std::println("Logging:");
    std::println("  --quiet              Only print errors");
    std::println("  --verbose            More detailed logs");
    std::println("  --debug              Debug logs (includes USB packet hexdumps)");
    std::println("");
    std::println("USB selection overrides (optional):");
    std::println("  --vid <hex>          Override USB vendor ID (hex, e.g. 04e8)");
    std::println("  --pid <hex>          Override USB product ID (hex)");
    std::println("  --usb-interface <n>  Force a specific USB interface number");
    std::println("");
    std::println("Linux permissions:");
    std::println("  If you get LIBUSB_ERROR_ACCESS, install the provided udev rule:");
    std::println("    udev/60-muninn.rules -> /etc/udev/rules.d/60-muninn.rules");
}

static void print_version() {
    std::println("{} version {} — forged for 爪卂丂ㄒ乇尺爪工刀ᗪ", tool_name(), muninn_get_version());
}

static void print_license() {
    std::println("{} — memory's raven. Samsung Thor protocol flasher", tool_name());
    std::println("");
    std::println("Copyright (c) 2026 Llucs");
    std::println("");
    std::println("Licensed under the Apache License, Version 2.0 (the \"License\");");
    std::println("you may not use this software except in compliance with the License.");
    std::println("You may obtain a copy of the License at:");
    std::println("");
    std::println("  http://www.apache.org/licenses/LICENSE-2.0");
    std::println("");
    std::println("This software is provided \"AS IS\", WITHOUT WARRANTIES OR CONDITIONS");
    std::println("OF ANY KIND, either express or implied.");
}

static auto parse_hex_u16(const std::string& s, uint16_t& out) -> bool {
    std::string v = s;
    if (v.starts_with("0x") || v.starts_with("0X")) {
        v = v.substr(2);
    }
    if (v.empty() || v.size() > 4) {
        return false;
    }
    for (unsigned char c : v) {
        if (std::isxdigit(c) == 0) {
            return false;
        }
    }
    try {
        out = static_cast<uint16_t>(std::stoul(v, nullptr, 16));
        return true;
    } catch (...) {
        return false;
    }
}

static auto parse_int(const std::string& s, int& out) -> bool {
    try {
        size_t idx = 0;
        int v = std::stoi(s, &idx, 10);
        if (idx != s.size()) {
            return false;
        }
        out = v;
        return true;
    } catch (...) {
        return false;
    }
}

static auto has_any_firmware_files(const OdinConfig& cfg) -> bool {
    return !cfg.bootloader.empty() || !cfg.ap.empty() || !cfg.cp.empty() || !cfg.csc.empty() || !cfg.ums.empty();
}

// Entry point for the muninn CLI.
//
// High-level flow:
// 1) Parse command-line options into an OdinConfig instance.
// 2) Handle informational options that terminate early (-h/-v/-w/-l).
// 3) Validate option combinations and required arguments.
// 4) Execute the requested device interaction / flashing workflow.
// 5) Return a process exit code indicating success or failure.
auto main(int argc, char** argv) -> int {
    OdinConfig cfg;
    bool list_only = false;

    // Phase 1: parse CLI arguments and apply them to cfg.
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];

        // Informational flags: print and exit immediately.
        if (arg == "-h") {
            print_usage();
            return 0;
        }
        if (arg == "-v") {
            print_version();
            return 0;
        }
        if (arg == "-w") {
            print_license();
            return 0;
        }

        if (arg == "--quiet") {
            cfg.quiet = true;
            cfg.verbose = false;
            cfg.debug = false;
            continue;
        }
        if (arg == "--verbose") {
            cfg.verbose = true;
            cfg.quiet = false;
            continue;
        }
        if (arg == "--debug") {
            cfg.debug = true;
            cfg.verbose = true;
            cfg.quiet = false;
            continue;
        }

        if (arg == "--reboot") {
            cfg.reboot = true;
            cfg.no_reboot = false;
            continue;
        }
        if (arg == "--no-reboot") {
            cfg.no_reboot = true;
            cfg.reboot = false;
            continue;
        }
        if (arg == "--json") {
            cfg.json = true;
            continue;
        }
        if (arg == "--redownload") {
            cfg.redownload = true;
            continue;
        }
        if (arg == "--efs-clear") {
            cfg.efs_clear = true;
            continue;
        }
        if (arg == "--bl-update") {
            cfg.boot_update = true;
            continue;
        }
        if (arg == "--check-only") {
            cfg.dry_run = true;
            continue;
        }
        if (arg == "--allow-unknown") {
            cfg.allow_unknown = true;
            continue;
        }

        auto take_value = [&](std::string& out) -> bool {
            if (i + 1 >= argc) {
                return false;
            }
            out = argv[++i];
            return true;
        };

        auto take_value_u16hex = [&](uint16_t& out) -> bool {
            if (i + 1 >= argc) {
                return false;
            }
            return parse_hex_u16(argv[++i], out);
        };

        auto take_value_int = [&](int& out) -> bool {
            if (i + 1 >= argc) {
                return false;
            }
            return parse_int(argv[++i], out);
        };

        if (arg == "--vid") {
            uint16_t v = 0;
            if (!take_value_u16hex(v)) {
                std::println(std::cerr, "Error: --vid requires a hex value");
                return 1;
            }
            cfg.has_vid = true;
            cfg.vid = v;
            continue;
        }
        if (arg == "--pid") {
            uint16_t p = 0;
            if (!take_value_u16hex(p)) {
                std::println(std::cerr, "Error: --pid requires a hex value");
                return 1;
            }
            cfg.has_pid = true;
            cfg.pid = p;
            continue;
        }
        if (arg == "--usb-interface") {
            int n = 0;
            if (!take_value_int(n) || n < 0 || n > 255) {
                std::println(std::cerr, "Error: --usb-interface requires an integer in the range 0..255");
                return 1;
            }
            cfg.has_usb_interface = true;
            cfg.usb_interface = n;
            continue;
        }
        if (arg == "--wait-device") {
            int secs = 0;
            if (!take_value_int(secs) || secs < 0) {
                std::println(std::cerr, "Error: --wait-device requires a non-negative integer (seconds)");
                return 1;
            }
            cfg.wait_device_secs = secs;
            continue;
        }

        if (arg == "-l") {
            list_only = true;
            continue;
        }

        if (arg == "-b" || arg == "-a" || arg == "-c" || arg == "-s" || arg == "-u" || arg == "-d") {
            std::string value;
            if (!take_value(value)) {
                std::println(std::cerr, "Error: Option requires an argument: {}", arg);
                return 1;
            }
            if (arg == "-b") {
                cfg.bootloader = value;
            } else if (arg == "-a") {
                cfg.ap = value;
            } else if (arg == "-c") {
                cfg.cp = value;
            } else if (arg == "-s") {
                cfg.csc = value;
            } else if (arg == "-u") {
                cfg.ums = value;
            } else if (arg == "-d") {
                cfg.device_path = value;
            }
            continue;
        }

        if (!arg.empty() && arg.starts_with('-')) {
            std::println(std::cerr, "Error: Unknown option: {}", arg);
            return 1;
        }
    }

    muninn_init(cfg);

    // Optional: poll until a Download Mode device appears (for scripting).
    if (cfg.wait_device_secs > 0 && cfg.device_path.empty()) {
        auto waited_for_device = [&]() -> bool {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(cfg.wait_device_secs);
            while (std::chrono::steady_clock::now() < deadline) {
                if (!muninn_list_devices(cfg).empty()) {
                    return true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(400));
            }
            return !muninn_list_devices(cfg).empty();
        };
        if (!waited_for_device()) {
            std::println(std::cerr, "Error: no Download Mode device appeared within {}s", cfg.wait_device_secs);
            return 2;
        }
    }

    if (list_only) {
        const std::vector<std::string> devices = muninn_list_devices(cfg);
        if (cfg.json) {
            std::println("{{\"tool\":\"{}\",\"devices\":[{}]}}", tool_name(), [&]() {
                std::string out;
                for (std::size_t i = 0; i < devices.size(); ++i) {
                    if (i > 0) out += ",";
                    out += "\"" + devices[i] + "\"";
                }
                return out;
            }());
        } else if (devices.empty()) {
            std::println("No devices detected in Download Mode.");
        } else {
            for (const auto& path : devices) {
                std::println("{}", path);
            }
        }
        return 0;
    }

    if (cfg.dry_run && !has_any_firmware_files(cfg)) {
        std::println(std::cerr, "Error: --check-only requires at least one firmware archive (-b/-a/-c/-s/-u)");
        return 1;
    }

    if (!has_any_firmware_files(cfg) && !cfg.reboot && !cfg.redownload) {
        print_usage();
        return 1;
    }

    for (const auto& path : {cfg.bootloader, cfg.ap, cfg.cp, cfg.csc, cfg.ums}) {
        if (!path.empty() && !std::filesystem::exists(path)) {
            std::println(std::cerr, "Error: Firmware file not found: {}", path);
            return 5; // Firmware error
        }
    }

    const int exit_code = static_cast<int>(muninn_run(cfg));

    if (cfg.json) {
        const bool do_reboot = !cfg.no_reboot && (cfg.reboot || has_any_firmware_files(cfg));
        std::println("{{\"tool\":\"{}\",\"status\":\"{}\",\"exit\":{},\"device\":\"{}\",\"reboot\":{},\"redownload\":{},\"files\":{{\"BL\":\"{}\",\"AP\":\"{}\",\"CP\":\"{}\",\"CSC\":\"{}\",\"UMS\":\"{}\"}}}}",
                     tool_name(), exit_code == 0 ? "ok" : "error", exit_code, cfg.device_path,
                     do_reboot ? "true" : "false", (cfg.redownload && !cfg.no_reboot) ? "true" : "false",
                     cfg.bootloader, cfg.ap, cfg.cp, cfg.csc, cfg.ums);
    }

    return exit_code;
}
