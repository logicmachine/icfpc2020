#include "galaxy.hpp"

using Vec = galaxy::Vec;

Vec gravity(const Vec p){
	const long abs_x = std::abs(p.x), abs_y = std::abs(p.y);
	if(abs_x > abs_y){
		return Vec(p.x > 0 ? -1 : 1, 0);
	}else if(abs_x < abs_y){
		return Vec(0, p.y > 0 ? -1 : 1);
	}else{
		return Vec(p.x > 0 ? -1 : 1, p.y > 0 ? -1 : 1);
	}
}

std::pair<Vec, Vec> simulate(const Vec& p0, const Vec& d0){
	const Vec g = gravity(p0);
	const Vec d(d0.x + g.x, d0.y + g.y);
	const Vec p(p0.x + d.x, p0.y + d.y);
	return std::make_pair(p, d);
}

bool universe_check(const Vec& p0, const Vec& d0, int n, const galaxy::StaticGameInfo& info){
	const long r = info.universe_radius;
	Vec p = p0, d = d0;
	for(int i = 0; i < n; ++i){
		const auto next = simulate(p, d);
		if(std::abs(next.first.x) > r){ return false; }
		if(std::abs(next.first.y) > r){ return false; }
		p = next.first;
		d = next.second;
	}
	return true;
}

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
	ship_params.x0 = (self_role == galaxy::PlayerRole::ATTACKER ? 510 : 446) - 120;  // TODO
	ship_params.x1 = 0;
	ship_params.x2 = 10;
	ship_params.x3 = 1;
	res = ctx.start(ship_params);

	// Command loop
	while(res.stage == galaxy::GameStage::RUNNING){
		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role != res.static_info.self_role){ continue; }
			if(ship.params.x0 <= 50){ continue; }  // TODO
			const Vec p = ship.pos, d = ship.vel;
			const Vec dd(p.x >= 0 ? -1 : 1, p.y >= 0 ? -1 : 1);
			const auto next = simulate(p, Vec(d.x - dd.x, d.y - dd.y));
			if(universe_check(next.first, next.second, 15, res.static_info)){
				cmds.accel(ship.id, dd);
			}
		}
		res = ctx.command(cmds);
	}

	galaxy::global_finalize();
	return 0;
}
