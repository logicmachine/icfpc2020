#include "../include/galaxy.hpp"
#include <cmath>

galaxy::Vec gravity_force(const galaxy::Vec& ship_pos, const galaxy::StaticGameInfo& info) {
  long r = info.galaxy_radius;
  long lx = -r, rx = r, uy = r, by = -r;
  long x = ship_pos.x, y = ship_pos.y;
  long cand_fx = (x < 0 ? 1 : -1);
  long cand_fy = (y < 0 ? 1 : -1);

  if (lx <= x && x <= rx) return galaxy::Vec(cand_fy, 0);
  if (by <= y && y <= uy) return galaxy::Vec(0, cand_fx);

  long dx = std::min(abs(x - lx), abs(x - rx));
  long dy = std::min(abs(y - uy), abs(y - by));

  if (dx > dy) {
    return galaxy::Vec(cand_fx, 0);
  }
  else if (dx < dy) {
    return galaxy::Vec(0, cand_fy);
  }
  else {
    return galaxy::Vec(cand_fx, cand_fy);
  }
}

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

bool universe_check(const Vec& p0, const Vec& d0, int n, long r){
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
  if (self_role == galaxy::PlayerRole::ATTACKER) {
    ship_params.x1 =  64;
    ship_params.x2 = 10;
    ship_params.x3 =  1;
    ship_params.x0 =
        res.static_info.parameter_capacity
        -  4 * ship_params.x1
        - 12 * ship_params.x2
        -  2 * ship_params.x3;
  }
  else {
    ship_params.x1 = 0;
    ship_params.x2 = 10;
    ship_params.x3 = 1;
    ship_params.x0 =
      res.static_info.parameter_capacity
      -  4 * ship_params.x1
      - 12 * ship_params.x2
      -  2 * ship_params.x3;
  }
  res = ctx.start(ship_params);

	// Command loop
	long counter = 0;
  long prev_attack_counter = 0;
  bool inKidou = false;

  const long breaking_pos = 3;
  const long kidou_vec = 5;
	while(res.stage == galaxy::GameStage::RUNNING){
		res.dump(std::cerr);

		galaxy::CommandListBuilder cmds;
    int eneId = -1;
    int myId = -1;
    bool attack = false;
    for (int i = 0; i < res.state.ships.size(); i++) {
      const auto& sac = res.state.ships[i];
      const auto& ship = sac.ship;
      if (ship.role != res.static_info.self_role) { eneId = i; continue; }
      else myId = i;
      if (ship.params.x0 <= 50) { continue; }  // TODO

      long r = res.static_info.galaxy_radius;
      long x = ship.pos.x;
      long y = ship.pos.y;
      galaxy::Vec gravAcc = gravity_force(ship.pos, res.static_info);

      if (inKidou || abs(x) <= breaking_pos || abs(y) <= breaking_pos) { // 軌道に入った？
        long long dx = 0, dy = 0;
        inKidou = true;
        if (abs(x) <= breaking_pos && abs(ship.vel.x) < kidou_vec) {
          dx = ship.vel.x < 0 ? 1 : -1;
        }
        if (abs(y) <= breaking_pos && abs(ship.vel.y) < kidou_vec) {
          dy = ship.vel.y < 0 ? 1 : -1;
        }
        cmds.accel(ship.id, galaxy::Vec(dx, dy));
      }
      else {
        long distx = abs(x);
        long disty = abs(y);

        long addx = 0, addy = 0;
        bool toY0 = false;
        const long accel_limit = 3;

        if (disty <= distx) toY0 = true;
        if (toY0) {
          if (x != 0) addx = (x < 0 ? 1 : -1);
          if (y != 0) addy = (y < 0 ? -1 : 1);
        }
        else {
          if (x != 0) addx = (x < 0 ? -1 : 1);
          if (y != 0) addy = (y < 0 ? 1 : -1);
        }
        long nextvx, nextvy;
        nextvx = ship.vel.x - addx + gravAcc.x;
        nextvy = ship.vel.y - addy + gravAcc.y;
        if (toY0) {
          if (abs(y) <= 3 && abs(nextvy) > 0) addy = -addy;
          else if (abs(nextvy) >= accel_limit) addy = 0; // 進行方向に加速しすぎない
        }
        else {
          if (abs(x) <= 3 && abs(nextvx) > 0) addx = -addx;
          if (abs(nextvx) >= accel_limit) addx = 0;
        }
        cmds.accel(ship.id, galaxy::Vec(addx, addy));
      }
    }
		res = ctx.command(cmds);
		++counter;
	}

	galaxy::global_finalize();
	return 0;
}
