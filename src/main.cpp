#include "declarations.hpp"
#include "tomlparse.hpp"
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <cerrno>

int main() {
    /* if(argc < 2){
        std::cout << "Not enough arguments: Will change this to run on files toward the end. Based on Nix flakes." << std::endl;
        return -1;
    } */
    
    std::vector<std::string> binaries = {"bwrap", "nix"}; // runtime dependencies for the program - incomplete.
    
    std::filesystem::path BWRAP_PATH = "";
    for(const std::string& i : binaries) {
        if(i == "bwrap" && pathfinder(i).empty()) {
            BWRAP_PATH = pathfinder("bubblewrap");
        } else if ( i == "bwrap" ) {
            BWRAP_PATH = pathfinder(i);
        }
        pathfinder(i);
    }
    if (BWRAP_PATH.empty()) {
        std::cerr << "Could not locate 'bwrap' or 'bubblewrap' binary in PATH" << std::endl;
        return 1;
    }

    std::filesystem::path config_file = find_file("bwrapconf.toml");
    if (config_file.empty()) {
        std::cerr << "Could not find 'bwrapconf.toml' configuration file" << std::endl;
        return 1;
    }

    std::vector<std::string> param_args_str = toml_parse(config_file);
    param_args_str.insert(param_args_str.begin(), BWRAP_PATH.string());


    // Lifetime safety: Convert std::string vector to C-style char* array right before execvp
    std::vector<char*> args;
    args.reserve(param_args_str.size() + 1);

    std::cout << "Constructed argument vector (" << param_args_str.size() << " elements):" << std::endl;
    for (size_t idx = 0; idx < param_args_str.size(); ++idx) {
        std::cout << "  [" << idx << "]: " << param_args_str[idx] << std::endl;
        args.push_back(const_cast<char*>(param_args_str[idx].c_str()));
    }
    args.push_back(nullptr); // execvp requires a NULL-terminated array

    std::cout << "[DEBUG] Launching execvp now..." << std::endl;
    std::cout << "--------------------------------------------" << std::endl;

    // Execute bwrap
    execvp(BWRAP_PATH.c_str(), args.data());

    // If execvp returns, execution failed
    int err = errno;
    std::cerr << "--------------------------------------------" << std::endl;
    std::cerr << "[ERROR] execvp failed! Error code " << err << ": " << std::strerror(err) << std::endl;
    
    return 1;
}
