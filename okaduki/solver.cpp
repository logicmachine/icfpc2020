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
	const auto self_role = res.static_info.self_role;

	// Start
	galaxy::ShipParams ship_params;
	ship_params.x0 = (self_role == galaxy::PlayerRole::ATTACKER ? 510 : 446);  // TODO
	ship_params.x1 = 0;
	ship_params.x2 = 0;
	ship_params.x3 = 1;
	res = ctx.start(ship_params);
	const long field_size = res.static_info.field_size;

	// Command loop
	const long interval = 4;
	long counter = 0;
	while(res.stage == galaxy::GameStage::RUNNING){
		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;
		if((counter + 1) % interval != 0){
			for(const auto& sac : res.state.ships){
				const auto& ship = sac.ship;
				if(ship.role != res.static_info.self_role){ continue; }

				if(ship.params.x0 <= 100){ continue; }  // TODO
				const long ddx = ship.pos.x >= 0 ? -1 : 1;
				const long ddy = ship.pos.y >= 0 ? -1 : 1;

				const galaxy::Vec npos(ship.pos.x + ddx, ship.pos.y + ddy);
				if (
					-field_size < npos.x && npos.x < field_size
					-field_size < npos.y && npos.y < field_size
				) {
					cmds.accel(ship.id, galaxy::Vec(ddx, ddy));
				}
			}
		}
		res = ctx.command(cmds);
		++counter;
	}

	galaxy::global_finalize();
	return 0;
}
