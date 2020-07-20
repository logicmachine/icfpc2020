#include "galaxy.hpp"

#include <limits>
#include <cmath>
#include <queue>
#include <tuple>
#include <map>

namespace galaxy {

Vec operator+(const Vec &v1, const Vec &v2) {
	return Vec(v1.x + v2.x, v1.y + v2.y);
}

Vec operator-(const Vec &v1, const Vec &v2) {
	return Vec(v1.x - v2.x, v1.y - v2.y);
}

}

//----------------------------------------------------------------------------
// Settings
//----------------------------------------------------------------------------
const int UNIVERSE_CHECK_ITERATIONS = 15;
const int RELATIVE_ANGLE_THRESHOLD = 4;

//----------------------------------------------------------------------------
// Utilities
//----------------------------------------------------------------------------
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

bool universe_check(const Vec& p0, const Vec& d0, int n, long ur, long gr){
	Vec p = p0, d = d0;
	for(int i = 0; i < n; ++i){
		const auto next = simulate(p, d);
		if(std::abs(next.first.x) > ur){ return false; }
		if(std::abs(next.first.y) > ur){ return false; }
		if(std::abs(next.first.x) <= gr && std::abs(next.first.y) <= gr){ break; }
		p = next.first;
		d = next.second;
	}
	return true;
}

Vec compute_accel(const Vec& p, const Vec& d, long ur, long gr){
	const Vec dd(p.x >= 0 ? -1 : 1, p.y >= 0 ? -1 : 1);
	const auto next = simulate(p, Vec(d.x - dd.x, d.y - dd.y));
	if(universe_check(next.first, next.second, UNIVERSE_CHECK_ITERATIONS, ur, gr)){
		return dd;
	}else{
		return Vec();
	}
}



bool alive(Vec pos, Vec vel, int turn, int rad) {

	for (int i = 0; i < turn; i++) {

		std::tie(pos, vel) = simulate(pos, vel);

		if (std::abs(pos.y) <= rad && std::abs(pos.x) <= rad) {
			return false;
		}
	}
	return true;
}

using State = std::tuple<int, int, Vec, Vec>;


std::vector<Vec> search_alive(const Vec init_pos, const Vec init_vel, int rad, int max_depth) {
	
	auto ss = std::make_tuple(0, -1, init_pos, init_vel);

	std::queue<State> que;
	que.push(ss);

	const std::array<Vec, 8> as = {
		Vec(-1, 1),
		Vec(-1, 0),
		Vec(-1, -1),
		Vec(0, -1),
		Vec(1, -1),
		Vec(1, 0),
		Vec(1, 1),
		Vec(0, 1)
	};

	std::map<State, State> prev;

	while (!que.empty()) {
		
		int depth, pai;
		Vec pos, vel;
		auto cs = que.front();
		std::tie(depth, pai, pos, vel) = cs;
		que.pop();

		for (int ai = 0; ai < 8; ai++) {
			auto a = as[ai];

			auto nvel = vel + gravity(pos) + a;
			auto npos = pos + nvel;

			auto ns = std::make_tuple(depth + 1, ai, npos, nvel);
			prev[ns] = cs;

			if (alive(npos, nvel, 256, rad)) {

				std::vector<Vec> recur;
				recur.push_back(as[ai]);

				while (std::get<1>(cs) != -1) {
					auto ps = prev[cs];
					recur.push_back(as[std::get<1>(ps)]);
					cs = ps;
				}
				return recur;
			}
		}
	}
	return std::vector<Vec>();
}

enum class MoveState {
	PREPARE_REVOLUTION,
	WAIT,
};


void move_attcker(galaxy::GalaxyContext &ctx, galaxy::GameResponse &res) {

	const auto self_role = res.static_info.self_role;

	MoveState move_state = MoveState::PREPARE_REVOLUTION;

	// Start
	galaxy::ShipParams ship_params;
	ship_params.x1 =  0;
	ship_params.x2 = 10;
	ship_params.x3 = 10;

	ship_params.x0 = 
		  res.static_info.parameter_capacity
		-  4 * ship_params.x1
		- 12 * ship_params.x2
		-  2 * ship_params.x3;
	res = ctx.start(ship_params);

	int base_shipid = 0;

	// Command loop
	long counter = 0;
	while(res.stage == galaxy::GameStage::RUNNING){

		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;

		// setup information
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if (counter == 0) {
				base_shipid = ship.id;
			}
		}

		// decide turn
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role == res.static_info.self_role){

				// shoot



				// move

			}
		}

		res = ctx.command(cmds);
		++counter;
	}

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

	if (self_role == galaxy::PlayerRole::ATTACKER) {

		move_attcker(ctx, res);

	} else {

		move_attcker(ctx, res);

	}
	galaxy::global_finalize();
	return 0;
}
