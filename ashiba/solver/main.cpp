#include "galaxy.hpp"

#include <algorithm>
#include <cmath>

const double PI = 3.1415926535;

const int ARG_MIN = 30 * PI/180;
const int ARG_MAX = 60 * PI/180;

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

		while(res.stage == galaxy::GameStage::RUNNING){
			std::vector<galaxy::Vec> enemyShipsPosVec;
			res.dump(std::cerr);
			galaxy::CommandListBuilder cmds;
			for(const auto& sac : res.state.ships){
				const auto& ship = sac.ship;
				if(ship.role != res.static_info.self_role){
					enemyShipsPosVec.emplace_back(ship.pos);
				} else {
					if(ship.params.x0 <= 50){ continue; }  // TODO

					if (std::max(ship.pos.x, ship.pos.y) < 25) { // 惑星が近い場合は逆向きに噴射
						const int ddx = ship.pos.x < ship.pos.y ? 0 : (ship.pos.x < 0 ? -1 : 1);
						const int ddy = ship.pos.x < ship.pos.y ? (ship.pos.y < 0 ? -1 : 1) : 0;
						cmds.accel(ship.id, galaxy::Vec(ddx, ddy));
					} else {
						double theta = std::asin(abs(ship.pos.y) / std::hypot(ship.pos.x, ship.pos.y));
						if (ARG_MIN <= theta <= ARG_MAX or (ship.vel.x == 0 and ship.vel.y == 0)) { // 惑星の対角線あたりにいるとき(または速度0のとき)は時計回りに加速する
							const int ddx = ship.pos.y > 0 ? 1 : -1;
							const int ddy = ship.pos.x > 0 ? 1 : -1;
							cmds.accel(ship.id, galaxy::Vec(ddx, ddy));
						} else {
							/*  TODO */;
						}
					}
				}
			}
			for(const auto& sac : res.state.ships){
				const auto& ship = sac.ship;
				if(ship.role != res.static_info.self_role){ continue; }
				if(ship.params.x0 <= 50){ continue; }  // TODO

				assert(enemyShipsPosVec.size() >= 1);
				cmds.shoot(ship.id, galaxy::Vec(enemyShipsPosVec[0].x, enemyShipsPosVec[0].y));
			}

			res = ctx.command(cmds);
		}
	} else {
		ship_params.x1 = 0;
		ship_params.x2 = 0;
		ship_params.x3 = 1;
		ship_params.x0 = 448 - (ship_params.x1 * 8 + ship_params.x2 * 16 + ship_params.x3 * 2);

		assert(isShipParamsValid(ship_params) == true);
		res = ctx.start(ship_params);

		while(res.stage == galaxy::GameStage::RUNNING){
			res.dump(std::cerr);
			galaxy::CommandListBuilder cmds;
			for(const auto& sac : res.state.ships){
				const auto& ship = sac.ship;
				if(ship.role != res.static_info.self_role){ continue; }
				if(ship.params.x0 <= 50){ continue; }  // TODO

				if (std::max(ship.pos.x, ship.pos.y) < 25) { // 惑星が近い場合は逆向きに噴射
					const int ddx = ship.pos.x < ship.pos.y ? 0 : (ship.pos.x < 0 ? -1 : 1);
					const int ddy = ship.pos.x < ship.pos.y ? (ship.pos.y < 0 ? -1 : 1) : 0;
					cmds.accel(ship.id, galaxy::Vec(ddx, ddy));
				} else {
					double theta = std::asin(abs(ship.pos.y) / std::hypot(ship.pos.x, ship.pos.y));
					if (ARG_MIN <= theta <= ARG_MAX or (ship.vel.x == 0 and ship.vel.y == 0)) { // 惑星の対角線あたりにいるとき(または速度0のとき)は時計回りに加速する
						const int ddx = ship.pos.y > 0 ? 1 : -1;
						const int ddy = ship.pos.x > 0 ? 1 : -1;
						cmds.accel(ship.id, galaxy::Vec(ddx, ddy));
					} else {
						/*  TODO */;
					}
				}
			}
			res = ctx.command(cmds);
		}
	}



	galaxy::global_finalize();
	return 0;
}
