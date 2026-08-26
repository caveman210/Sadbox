#include "declarations.hpp"
#include <unistd.h>

int main(int argc, char* argv[]){
	if(argc < 2){
		std::cout << "Not enough arguments: Will change this to run on files toward the end. Based on Nix flakes." << std::endl;
		return -1;
	}
	std::vector<std::string> binaries = {"bwrap", "nix"};	// runtime dependencies for the program - incomplete.
	
	std::filesystem::path BWRAP_PATH = "";
	for(std::string i : binaries){
		if(i == "bwrap" && pathfinder(i).empty()){
			BWRAP_PATH = pathfinder("bubblewrap");
		} else if ( i == "bwrap" ) {
			BWRAP_PATH = pathfinder(i);
		}
		pathfinder(i);
	}

	std::vector<std::string> param_args_str = argparse(argc, argv);
	// Code snippet begin
	std::vector<char*> args;

	for(auto& ar: param_args_str){
		args.push_back(ar.data());
	}

	args.push_back(nullptr);
	/* code snippet end
	 * This is a very very tricky area speaking of lifetimes.
	 * Placed it initially at the end of argparse fn. 
	 * Debugging (AI) told me it was causing lifetime issues
	 * An immediate fix would be 'static', but that's dumb. */

	execvp(BWRAP_PATH.c_str(), args.data());
	std::cerr << ("Failed to execute bwrap.\n");
	return 0;
}
