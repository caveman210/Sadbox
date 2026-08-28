/* *
 * Gave up on trying to make my own TOML parser as its just too much load on my peabrain.
 * Don't ask me how I made this parser, I did not make this. Maybe I will rewrite it in the future if needs be.
 * */

#include "tomlparse.hpp"
#include <toml++/toml.hpp>
#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <sstream>
#include <vector>
#include <string>

std::filesystem::path find_file(const std::string& file) {
    if (file.empty()) {
        std::cerr << "[ERROR] No filename provided to find_file()." << std::endl;
        return "";
    }

    std::filesystem::path currentDir = std::filesystem::current_path();
    
    // Check 1: ./configurations/file
    std::filesystem::path targetFile = currentDir / "configurations" / file;
    if (std::filesystem::exists(targetFile)) {
        return targetFile;
    }

    // Check 2: ../configurations/file (for binaries running inside build/)
    std::filesystem::path parentTarget = currentDir.parent_path() / "configurations" / file;
    if (std::filesystem::exists(parentTarget)) {
        return parentTarget;
    }

    // Check 3: Current working directory
    if (std::filesystem::exists(currentDir / file)) {
        return currentDir / file;
    }

    std::cerr << "[ERROR] Could not locate config file: " << file << std::endl;
    return "";
}

// Safe helper to process paired array entries (e.g. ro-bind, bind, dev-bind)
static void parse_bind_table(const toml::table& tbl, const char* section, const char* flag, std::vector<std::string>& params) {
    if (tbl["mounts"][section]["set"].value_or(false)) {
        if (auto entries = tbl["mounts"][section]["entries"].as_table()) {
            for (auto&& [name, pair] : *entries) {
                if (auto path_arr = pair.as_array()) {
                    if (path_arr->size() >= 2) {
                        auto src = path_arr->get(0)->as_string();
                        auto dst = path_arr->get(1)->as_string();
                        if (src && dst) {
                            params.push_back(flag);
                            params.push_back(src->get());
                            params.push_back(dst->get());
                        }
                    }
                }
            }
        }
    }
}

std::vector<std::string> toml_parse(std::filesystem::path path) {
    std::vector<std::string> params;
    toml::table tbl;

    if (path.empty() || !std::filesystem::exists(path)) {
        std::cerr << "Config Error: Invalid path provided to parser." << std::endl;
        return {};
    }

    // 1. Safe TOML File Loading
    try {
        tbl = toml::parse_file(path.c_str());
    } catch (const toml::parse_error& err) {
        std::cerr << "Config Error: Failed to parse TOML (" << err.description() << ")\n";
        return {};
    }

    // ==========================================
    // STAGE 1: Namespaces & Global Controls
    // ==========================================
    if (tbl["unshare"]["unshare-all"]["set"].value_or(false)) {
        params.push_back("--unshare-all");
    }
    if (tbl["unshare"]["share-net"]["set"].value_or(false)) {
        params.push_back("--share-net");
    }

    if (!tbl["unshare"]["unshare-all"]["set"].value_or(false)) {
        const std::pair<const char*, const char*> unshare_specifics[] = {
            {"unshare-user-try", "--unshare-user-try"},
            {"unshare-ipc", "--unshare-ipc"},
            {"unshare-net", "--unshare-net"},
            {"unshare-uts", "--unshare-uts"},
            {"unshare-cgroup", "--unshare-cgroup"},
            {"unshare-cgroup-try", "--unshare-cgroup-try"}
        };
        for (const auto& [key, flag] : unshare_specifics) {
            if (tbl["unshare-specific"][key]["set"].value_or(false)) {
                params.push_back(flag);
            }
        }
    }

    // Process sub-tables under unshare-specific
    if (tbl["unshare-specific"]["unshare-user"]["set"].value_or(false)) {
        params.push_back("--unshare-user");
    }
    if (tbl["unshare-specific"]["unshare-pid"]["set"].value_or(false)) {
        params.push_back("--unshare-pid");
    }
    if (auto uid = tbl["unshare-specific"]["unshare-user"]["uid"].value<std::string>()) {
        if (!uid->empty()) { params.push_back("--uid"); params.push_back(*uid); }
    }
    if (auto gid = tbl["unshare-specific"]["unshare-user"]["gid"].value<std::string>()) {
        if (!gid->empty()) { params.push_back("--gid"); params.push_back(*gid); }
    }
    if (auto host = tbl["unshare-specific"]["unshare-uts"]["hostname"].value<std::string>()) {
        if (!host->empty()) { params.push_back("--hostname"); params.push_back(*host); }
    }

    // ==========================================
    // STAGE 2: Environment Setup
    // ==========================================
    if (tbl["env_var"]["clearenv"]["set"].value_or(false)) {
        params.push_back("--clearenv");
    }

    if (tbl["env_var"]["setenv"]["set"].value_or(false)) {
        if (auto vars = tbl["env_var"]["setenv"]["variables"].as_table()) {
            for (auto&& [key, val] : *vars) {
                if (auto str_val = val.as_string()) {
                    params.push_back("--setenv");
                    params.push_back(std::string(key.str()));
                    params.push_back(str_val->get());
                }
            }
        }
    }

    // ==========================================
    // STAGE 3: Base System Mounts
    // ==========================================
    if (tbl["mounts"]["proc"]["set"].value_or(false)) {
        params.push_back("--proc");
        params.push_back(tbl["mounts"]["proc"]["dest"].value_or("/proc"));
    }
    if (tbl["mounts"]["dev"]["set"].value_or(false)) {
        params.push_back("--dev");
        params.push_back(tbl["mounts"]["dev"]["dest"].value_or("/dev"));
    }

    // Process all variations of bind mounts safely
    parse_bind_table(tbl, "bind", "--bind", params);
    parse_bind_table(tbl, "bind-try", "--bind-try", params);
    parse_bind_table(tbl, "dev-bind", "--dev-bind", params);
    parse_bind_table(tbl, "dev-bind-try", "--dev-bind-try", params);
    parse_bind_table(tbl, "ro-bind", "--ro-bind", params);
    parse_bind_table(tbl, "ro-bind-try", "--ro-bind-try", params);

    // ==========================================
    // STAGE 4: Ephemeral Overlays & User Mounts
    // ==========================================
    if (tbl["mounts"]["tmpfs"]["set"].value_or(false)) {
        if (auto arr = tbl["mounts"]["tmpfs"]["destinations"].as_array()) {
            for (auto&& elem : *arr) {
                if (auto dest = elem.as_string()) {
                    params.push_back("--tmpfs");
                    params.push_back(dest->get());
                }
            }
        }
    }

    // ==========================================
    // STAGE 5: Working Dir & Lifecycle Process
    // ==========================================
    if (tbl["chdir"]["set"].value_or(false)) {
        if (auto dir = tbl["chdir"]["dir"].value<std::string>()) {
            if (!dir->empty()) {
                params.push_back("--chdir");
                params.push_back(*dir);
            }
        }
    }

    if (tbl["process"]["new-session"]["set"].value_or(false)) {
        params.push_back("--new-session");
    }
    if (tbl["process"]["die-with-parent"]["set"].value_or(false)) {
        params.push_back("--die-with-parent");
    }
    if (tbl["process"]["as-pid-1"]["set"].value_or(false)) {
        params.push_back("--as-pid-1");
    }

    if (auto shell = tbl["source"]["target"].value<std::string>()) {
        if (!shell->empty()) { params.push_back(*shell); }
    }


    return params;
}
