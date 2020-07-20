#include "galaxy.hpp"

#include <limits>
#include <cmath>
#include <queue>
#include <tuple>
#include <map>
#include <algorithm>

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

int distance(const Vec &v1, const Vec &v2) {
	return std::max(std::abs(v1.x - v2.x), std::abs(v1.y - v2.y));
}

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

bool alive(Vec pos, Vec vel, int turn, int rad, bool dump = false) {

	for (int i = 0; i < turn; i++) {

		if (dump) {
			std::cout << pos.x << ", " << pos.y << std::endl;
		}

		std::tie(pos, vel) = simulate(pos, vel);

		if (std::abs(pos.y) <= rad && std::abs(pos.x) <= rad) {
			return false;
		}
	}
	return true;
}

using State = std::tuple<int, int, Vec, Vec>;


std::vector<Vec> search_alive(const Vec init_pos, const Vec init_vel, int rad, int max_depth, int max_turn) {
	
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

			const auto nvel = vel + gravity(pos) - a;
			const auto npos = pos + nvel;

			auto ns = std::make_tuple(depth + 1, ai, npos, nvel);

			// add variance
			if (2 <= distance(nvel, init_vel) && alive(npos, nvel, max_turn, rad + 2)) {
				
				prev[ns] = cs;

				std::vector<Vec> recur;
				recur.push_back(as[ai]);

				while (true) {
					auto ps = prev[ns];
					if (std::get<1>(ps) == -1) {
						break;
					}
					recur.push_back(as[std::get<1>(ps)]);
					ns = ps;
				}

				std::reverse(recur.begin(), recur.end());
				assert(recur.size() == depth + 1);
				return recur;
			}

			if (depth + 1 <= max_depth && prev.find(ns) == prev.end()) {
				que.push(ns);
			}
			prev[ns] = cs;
		}
	}
	return std::vector<Vec>();
}

enum class MoveState {
	PREPARE_REVOLUTION,
	PREPARE_FORK,
	SLEEP,
	WAIT,
};


void move_attack(galaxy::GalaxyContext &ctx, galaxy::GameResponse &res) {

	const auto self_role = res.static_info.self_role;

	const int target_ship_count = 100;
	const int target_power = 0;
	const int target_repair = 10;

	// Start
	galaxy::ShipParams ship_params;
	ship_params.x1 =  target_power;
	ship_params.x2 = target_repair;
	ship_params.x3 = target_ship_count;

	ship_params.x0 = 
		  res.static_info.parameter_capacity
		-  4 * ship_params.x1
		- 12 * ship_params.x2
		-  2 * ship_params.x3;
	res = ctx.start(ship_params);

	const auto rad = res.static_info.galaxy_radius;

	std::map<int, MoveState> shipid_state;
	std::map<int, int> sleep_turn;

	int base_shipid = 0;
	int ship_count = 1;

	std::map<int, std::vector<Vec>> move_cache;

	// Command loop
	long counter = 0;
	while(res.stage == galaxy::GameStage::RUNNING){

		if (counter % 10 == 0) {
			std::cout << "turn: " << counter << std::endl;
		}

		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;

		// setup information
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if (counter == 0) {
				base_shipid = ship.id;
				shipid_state[base_shipid] = MoveState::PREPARE_REVOLUTION;
			}
		}

		// decide turn
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role == res.static_info.self_role){
				
				// register
				if (shipid_state.find(ship.id) == shipid_state.end()) {
					shipid_state[ship.id] = MoveState::WAIT;
				}

				if (shipid_state[ship.id] == MoveState::SLEEP) {
					if (--sleep_turn[ship.id] == 0) {
						shipid_state[ship.id] = MoveState::PREPARE_REVOLUTION;
					}
				}

				// move
				if (shipid_state[ship.id] == MoveState::PREPARE_REVOLUTION) {

					if (move_cache[ship.id].empty()) {
						move_cache[ship.id] = search_alive(ship.pos, ship.vel, rad, 7, res.static_info.time_limit);
						assert(!move_cache[ship.id].empty());
					}

					cmds.accel(ship.id, move_cache[ship.id].back());
					move_cache[ship.id].pop_back();
					if (move_cache[ship.id].empty()) {
						if (ship.id == base_shipid) {
							shipid_state[ship.id] = MoveState::PREPARE_FORK;
						} else {
							shipid_state[ship.id] = MoveState::WAIT;
						}
					}
				}

				if (shipid_state[ship.id] == MoveState::PREPARE_FORK) {
					galaxy::ShipParams params;
					params.x0 = 0;
					params.x1 = 0;
					params.x2 = 0;
					params.x3 = 1;
					cmds.fork(ship.id, params);
					ship_count++;
					if (target_ship_count == ship_count) {
						shipid_state[ship.id] = MoveState::WAIT;
					} else {
						shipid_state[ship.id] = MoveState::SLEEP;
						sleep_turn[ship.id] = 3;
					}
				}

				if (shipid_state[ship.id] == MoveState::WAIT) {

					// todo: select best 
					for (auto &op_ship_sac : res.state.ships) {
						auto &op_ship = op_ship_sac.ship;
						if (op_ship.role != res.static_info.self_role)	{
							if (ship.id != base_shipid && distance(op_ship.pos, ship.pos) <= 3) {
								cmds.detonate(ship.id);
							}
						}
					}
				}
			}
		}

		res = ctx.command(cmds);
		++counter;
	}

}

void move_defense(galaxy::GalaxyContext &ctx, galaxy::GameResponse &res) {
	
	// Start
	galaxy::ShipParams ship_params;
	ship_params.x1 = 0;
	ship_params.x2 = 8;
	ship_params.x3 = 1;
	ship_params.x0 =
		  res.static_info.parameter_capacity
		-  4 * ship_params.x1
		- 12 * ship_params.x2
		-  2 * ship_params.x3;

	res = ctx.start(ship_params);

	// Command loop
	while(res.stage == galaxy::GameStage::RUNNING){
		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role != res.static_info.self_role){ continue; }
			const auto accel = compute_accel(
				ship.pos, ship.vel,
				res.static_info.universe_radius,
				res.static_info.galaxy_radius);
			if(accel.x != 0 || accel.y != 0){
				cmds.accel(ship.id, accel);
			}
		}
		res = ctx.command(cmds);
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

		move_attack(ctx, res);

	} else {

		move_defense(ctx, res);

	}
	galaxy::global_finalize();
	return 0;
}
