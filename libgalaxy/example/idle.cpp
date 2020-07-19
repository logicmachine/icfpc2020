#include "galaxy.hpp"

int main(int argc, char *argv[]){
	if(argc < 3){
		std::cerr << "Usage: " << argv[0] << " endpoint player_key" << std::endl;
		return 0;
	}

	galaxy::global_initialize();

	const std::string endpoint = argv[1];
	const long player_key = atol(argv[2]);

	galaxy::GalaxyContext ctx(endpoint, player_key);

	// Join
	ctx.join();

	// Start
	galaxy::StartParams start_params;
	start_params.values[0] = 0;
	start_params.values[1] = 0;
	start_params.values[2] = 0;
	start_params.values[3] = 1;
	ctx.start(start_params);

	galaxy::global_finalize();
	return 0;
}
