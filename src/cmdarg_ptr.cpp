/*
 * Fetches me the cmdline arguments - yes it is what it is, don't judge me
 * */

#include "declarations.hpp"

std::vector<std::string> argparse(int argc, char* argv[]){
	if(argc < 2){
		std::cout << "No params to convert as ptrs." << std::endl;
		exit(EXIT_FAILURE);
	}

	std::vector<std::string> param_args_str;
	for(int i=1; i<argc; i++){
		param_args_str.push_back(argv[i]);
	}
	return param_args_str;
}


// exit(EXIT_FAILURE) is not the best case scenario. Hence pass custom destructors by making these into classes??
