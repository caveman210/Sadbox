/*
 * Basically which but not on steroids, less efficient and checks only binaries. Works cross-Linux tho :)
*/
#include "declarations.hpp"
#include <sstream>

std::filesystem::path pathfinder(std::string bin){
	if(bin == ""){
		std::cerr << "No executable specified." << std::endl;
		exit(EXIT_FAILURE);
	}

	if(const char* path = std::getenv("PATH")){
		std::string path_str = std::string(path);
		std::stringstream ss(path_str);
		std::string dir;

		while(std::getline(ss, dir, ':')){
			std::filesystem::path result_trial = std::filesystem::path(dir)/bin;
			if(std::filesystem::exists(result_trial)){
				return result_trial;
			}
			// std::cerr << "Unable to find binary: " << result_trial << std::endl;
		}
		// std::cerr << "Unable to find binary" << std::endl;
		exit(EXIT_FAILURE);
	}
}

// exit(EXIT_FAILURE) is not the best case scenario. Hence pass custom destructors by making these into classes??
