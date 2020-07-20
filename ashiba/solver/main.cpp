#include "galaxy.hpp"

#include <algorithm>
#include <cmath>
#include <vector>
#include <queue>
#include <set>

const double PI = 3.1415926535;
const double EPS = 1e-9;

const int FIELD_RADIUS = 128;
const int PLANET_RADIUS = 16;

const int ARG_MIN = 25 * PI/180;
const int ARG_MAX = 65 * PI/180;

bool isShipParamsValid(const galaxy::ShipParams& prms) {
if (prms.x0 < 0) return false;
if (prms.x1 < 0) return false;
if (prms.x2 < 0) return false;
if (prms.x3 <= 0) return false;
if (prms.x0 * 1
	+ prms.x1 * 8
	+ prms.x2 * 16
	+ prms.x3 *2
	> 512) return false;
	return true;
}


const int PLANET_R = 16;
galaxy::Vec calcGravity(const galaxy::Vec& pos) {
	const int ddx = pos.x < -PLANET_R ? 1 : -1;
	const int ddy = pos.y < -PLANET_R ? 1 : -1;

	if (-PLANET_R <= pos.x and pos.x <= PLANET_R) return galaxy::Vec(0, ddy);
	if (-PLANET_R <= pos.y and pos.y <= PLANET_R) return galaxy:: Vec(ddx, 0);

	const int diffx = std::min(abs(pos.x - PLANET_R), abs(pos.x - (-PLANET_R)));
	const int diffy = std::min(abs(pos.y - PLANET_R), abs(pos.y - (-PLANET_R)));

	if (diffx > diffx) return galaxy::Vec(ddx, 0);
	else if (diffx < diffx) return galaxy::Vec(0, ddy);
	else return galaxy::Vec(ddx, ddy);
}

galaxy::Vec calcNextVel(const galaxy::Vec& pos, const galaxy::Vec& vel) {
	galaxy::Vec res = calcGravity(pos);
	return galaxy::Vec(vel.x + res.x, vel.y + res.y);
}

galaxy::Vec calcNextPos(const galaxy::Vec& pos, const galaxy::Vec& vel) {
	galaxy::Vec res = calcNextVel(pos, vel);
	return galaxy::Vec(pos.x + res.x, pos.y + res.y);
}

std::vector<galaxy::Vec> predictFuturePos(const galaxy::Vec& cur_pos, const galaxy::Vec& cur_vel, const int& times) {
	std::vector<galaxy::Vec> positions(times+1);
	std::vector<galaxy::Vec> velocities(times+1);
	positions[0] = cur_pos;
	velocities[0] = cur_vel; 

	for (int i=1; i<=times; ++i) {
		velocities[i] = calcNextVel(positions[i-1], velocities[i-1]);
		positions[i]  = calcNextPos(positions[i-1], velocities[i]);
	}

	return positions;
}

bool isGrayArea(galaxy::Vec vec) {
	if (vec.x < -FIELD_RADIUS or vec.x > FIELD_RADIUS) return false;
	if (vec.y < -FIELD_RADIUS or vec.y > FIELD_RADIUS) return false;
	return true;
}

bool isIntoOrbit(const galaxy::Vec& pos, const galaxy::Vec& vel) {
	std::vector<galaxy::Vec> pos_vec = predictFuturePos(pos, vel, 100);

	auto comparison_func = [](const galaxy::Vec& a, const galaxy::Vec& b) { return std::make_pair(a.x, a.y) < std::make_pair(b.x, b.y); };
	std::set<galaxy::Vec, std::function<bool (const galaxy::Vec&, const galaxy::Vec&)>> visited(comparison_func);
	std::vector<bool> is_second_visit(pos_vec.size(), false);
	for (int i=0; i<pos_vec.size(); ++i) {
		is_second_visit[i] = visited.count(pos_vec[i]) == 0 ? false : true;
		visited.insert(pos_vec[i]);
	}

	int back_second_visit_count = 0;
	for (int i=is_second_visit.size()-1; i>=0; --i) {
		if (is_second_visit[i]) back_second_visit_count++;
		else break;
	}


	// std::cerr << pos_vec.size() << std::endl;
	// for (const auto& elm: pos_vec) {
	// 	std::cerr << elm.x << " " << elm.y << std::endl;
	// }
	
	for (const auto& elm: pos_vec) {
		if (abs(elm.x) < 16 or abs(elm.y) < 16 ) return false;
	}
	return (back_second_visit_count >=3);
}

void dfs(const int depth, const galaxy::Vec& pos, const galaxy::Vec& vel, std::vector<galaxy::Vec>& move, bool& solved) {
	if (solved) return ;
	if (depth >= 6) {
		// for (const auto& elm: move) {
		// 	std::cout << "(" << elm.x << ", " << elm.y << "), ";
		// }
		// std::cout << pos.x << " " << pos.y;
		// std::cout << std::endl;
		return ;
	}
	if (isIntoOrbit(pos, vel)) {
		std::cerr << "at depth = " << depth << std::endl;
		solved = true;
		return ;
	}
	for (int i=0; i<9; ++i) {
		const int dx = i%3 - 1;
		const int dy = i/3 - 1;
		
		move.push_back(galaxy::Vec(dx, dy));
		galaxy::Vec next_vel = calcNextVel(pos, vel);
		next_vel = galaxy::Vec(next_vel.x+dx, next_vel.y+dy);
		galaxy::Vec next_pos = calcNextPos(pos, next_vel);
		dfs(depth+1, next_pos, next_vel, move, solved);
		move.pop_back();
	}
}

std::queue<galaxy::Vec> getFutureMove(const galaxy::Vec& pos, const galaxy::Vec& vel) {
	std::vector<galaxy::Vec> move;
	int depth = 0;
	bool solved = false;
	dfs(depth, pos, vel, move, solved);

	if (solved == false) {
		std::cerr << "solution not found" << std::endl;
		return std::queue<galaxy::Vec>();
	}

	std::queue<galaxy::Vec> ret;
	for (const auto& elm: move) ret.push(elm);

	std::cerr << "solution found" << std::endl;
	return ret;
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

	if (self_role == galaxy::PlayerRole::ATTACKER) {
		ship_params.x1 = 5;
		ship_params.x2 = 0;
		ship_params.x3 = 1;
		ship_params.x0 = 512 - (ship_params.x1 * 8 + ship_params.x2 * 16 + ship_params.x3 * 2);

		assert(isShipParamsValid(ship_params) == true);
		res = ctx.start(ship_params);

		std::vector<std::queue<galaxy::Vec>> future_move;
		std::vector<bool> isShipIntoOrbit;


		while(res.stage == galaxy::GameStage::RUNNING){
			int count = 0;
			res.dump(std::cerr);
			galaxy::CommandListBuilder cmds;
			for(const auto& sac : res.state.ships){
				const auto& ship = sac.ship;
				if(ship.role != res.static_info.self_role){ continue; }
				if(ship.params.x0 <= 50){ continue; }  // TODO

				if (isShipIntoOrbit.size() <= ship.id or isShipIntoOrbit[ship.id] == false) {
					std::queue<galaxy::Vec> que = getFutureMove(ship.pos, ship.vel);
					while (future_move.size() <= ship.id)  future_move.emplace_back(std::queue<galaxy::Vec>());
					while (isShipIntoOrbit.size() <= ship.id) isShipIntoOrbit.push_back(false);

					future_move[ship.id] = que;
					isShipIntoOrbit[ship.id] = true;
				}

				std::cerr << "------------" << future_move[ship.id].size() << std::endl;
				if (not future_move[ship.id].empty()) {
					cmds.accel(ship.id, future_move[ship.id].front());
					future_move[ship.id].pop();
					std::cout << future_move[ship.id].size() << " " << future_move[ship.id].front().x << " " << future_move[ship.id].front().y << std::endl;
				}
			}
			count++;

			res = ctx.command(cmds);
		}
	} else {
		ship_params.x1 = 0;
		ship_params.x2 = 0;
		ship_params.x3 = 1;
		ship_params.x0 = 448 - (ship_params.x1 * 8 + ship_params.x2 * 16 + ship_params.x3 * 2);

		assert(isShipParamsValid(ship_params) == true);
		res = ctx.start(ship_params);

		// while(res.stage == galaxy::GameStage::RUNNING){
		// 	res.dump(std::cerr);
		// 	galaxy::CommandListBuilder cmds;
		// 	for(const auto& sac : res.state.ships){
		// 		const auto& ship = sac.ship;
		// 		if(ship.role != res.static_info.self_role){ continue; }
		// 		if(ship.params.x0 <= 50){ continue; }  // TODO

		// 		if (std::max(abs(ship.pos.x), abs(ship.pos.y)) < 35) { // 惑星が近い場合は逆向きに噴射
		// 			const int ddx = abs(ship.pos.x) < abs(ship.pos.y) ? 0 : (ship.pos.x < 0 ? -1 : 1);
		// 			const int ddy = abs(ship.pos.x) < abs(ship.pos.y) ? (ship.pos.y < 0 ? -1 : 1) : 0;
		// 			cmds.accel(ship.id, galaxy::Vec(ddx, ddy));
		// 		} else if (std::max(abs(ship.pos.x), abs(ship.pos.y)) >= 100) { // 枠外が近い場合は惑星向きに噴射
		// 			const int ddx = abs(ship.pos.x) < abs(ship.pos.y) ? 0 : (ship.pos.x < 0 ? -1 : 1);
		// 			const int ddy = abs(ship.pos.x) < abs(ship.pos.y) ? (ship.pos.y < 0 ? -1 : 1) : 0;
		// 			cmds.accel(ship.id, galaxy::Vec(ddx, ddy));
		// 		} else {
		// 			double theta = std::asin(abs(ship.pos.y) / std::hypot(ship.pos.x, ship.pos.y));
		// 			assert(theta >= 0-EPS and theta <= 90 + EPS);
		// 			if (theta < ARG_MIN or ARG_MAX < theta or (ship.vel.x == 0 and ship.vel.y == 0)) { // 惑星の対角線あたり以外にいるとき(または速度0のとき)は時計回りに加速する
		// 			const int ddx = abs(ship.pos.x) < abs(ship.pos.y) ? (ship.pos.x < 0 ? -1 : 1) : 0;
		// 			const int ddy = abs(ship.pos.x) < abs(ship.pos.y) ? 0 : (ship.pos.y > 0 ? -1 : 1);
		// 				cmds.accel(ship.id, galaxy::Vec(ddx, ddy));
		// 			} else {
		// 				/*  TODO */;
		// 			}
		// 		}
		// 	}
		// 	res = ctx.command(cmds);
		// }
	}



	galaxy::global_finalize();
	return 0;
}
