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

	// Response
	galaxy::GameResponse res;

	// Join
	res = ctx.join();

	// Start
	galaxy::ShipParams ship_params;
	ship_params.x0 = 0;
	ship_params.x1 = 0;
	ship_params.x2 = 0;
	ship_params.x3 = 1;
	res = ctx.start(ship_params);

	// Command loop
	while(res.stage == galaxy::GameStage::RUNNING){
		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;
		res = ctx.command(cmds);
	}

	galaxy::global_finalize();
	return 0;
}
