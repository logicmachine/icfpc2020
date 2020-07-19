#include <iostream>
#include <regex>
#include <string>
//#include "httplib.h"
#include "galaxy.hpp"

using namespace std;

class Solver {
private:

public:
	void solver() {
		return;
	}

};

int main(int argc, char* argv[])
{
	if (argc < 3) {
		std::cerr << "Usage: " << argv[0] << " endpoint player_key" << std::endl;
		return 0;
	}

	galaxy::global_initialize();
	
	/*
	https://icfpc2020-api.testkontur.ru/aliens/send
	b0a3d915b8d742a39897ab4dab931721
	*/

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
	while (res.stage == galaxy::GameStage::RUNNING) {
		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;
		res = ctx.command(cmds);
	}

	galaxy::global_finalize();
	
	return 0;
}
