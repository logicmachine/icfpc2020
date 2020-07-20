#include "../include/galaxy.hpp"
#include <cmath>
#include <map>

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

int dead_check(const Vec& p0, const Vec& d0, int n, long r){
	Vec p = p0, d = d0;
	for(int i = 0; i < n; ++i){
		const auto next = simulate(p, d);
		if(std::abs(next.first.x) <= r && std::abs(next.first.y) <= r){ return i; }
		p = next.first;
		d = next.second;
	}
	return n;
}

class SimulateN {
public:
  int rem_turn;
  int r;
  int dead_turn;
  int enddepth;
  std::vector<Vec> ans;
  std::vector<Vec> accels;

  SimulateN (int rem_turn, int r, int enddepth) : rem_turn(rem_turn), r(r), dead_turn(0), enddepth(enddepth) {}
  void simulate_n(const Vec& pos, const Vec& vec, int depth) {
    if (depth >= enddepth) {
      int now_deadturn = dead_check(pos, vec, rem_turn, r);
      if (dead_turn < now_deadturn) {
        ans = accels;
        dead_turn = now_deadturn;
      }
      return;
    }
    for (int dx = -1; dx <= 1; dx++) {
      for (int dy = -1; dy <= 1; dy++) {
        Vec acceled_vec(vec.x - dx, vec.y - dy);
        auto pair = simulate(pos, acceled_vec);
        accels.emplace_back(dx, dy);
        simulate_n(pair.first, pair.second, depth+1);
        accels.pop_back();
      }
    }
  }
};

class ShipKidouState {
public:
  bool is_on_kidou = false;
  long kidou_idx = -1;
  long shipId = -1;
  std::vector<Vec> kidou_plan_accel;
};

// X0 または Y0方向に低速度で近づける
Vec toX0orY0(const galaxy::ShipState& ship, const galaxy::StaticGameInfo& static_info) {
  galaxy::Vec grav_acc = gravity_force(ship.pos, static_info);
  long x = ship.pos.x;
  long y = ship.pos.y;
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
  nextvx = ship.vel.x - addx + grav_acc.x;
  nextvy = ship.vel.y - addy + grav_acc.y;
  if (toY0) {
    if (abs(y) <= 3 && abs(nextvy) > 0) addy = -addy;
    else if (abs(nextvy) >= accel_limit) addy = 0; // 進行方向に加速しすぎない
  }
  else {
    if (abs(x) <= 3 && abs(nextvx) > 0) addx = -addx;
    if (abs(nextvx) >= accel_limit) addx = 0;
  }
  return Vec(addx, addy);
}

// なんとなく動かす（惑星付近にいる時に端に逃げるように）
Vec escape_from_galaxy(const galaxy::ShipState& ship, const long near_galaxy_dist) {
  long long dx = 0, dy = 0;
  const long kidou_vec = 5;
  if (abs(ship.pos.x) <= near_galaxy_dist && abs(ship.vel.x) < kidou_vec) {
    dx = ship.vel.x < 0 ? 1 : -1;
  }
  if (abs(ship.pos.y) <= near_galaxy_dist && abs(ship.vel.y) < kidou_vec) {
    dy = ship.vel.y < 0 ? 1 : -1;
  }
  return Vec(dx, dy);
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
    ship_params.x2 = 8;
    ship_params.x3 = 50;
    ship_params.x0 =
      res.static_info.parameter_capacity
      -  4 * ship_params.x1
      - 12 * ship_params.x2
      -  2 * ship_params.x3;
  }
  res = ctx.start(ship_params);

	// Command loop
	long counter = 0;
  const long near_galaxy_dist = 4;
  std::map<long, ShipKidouState> ship_state_dic;
  
	while(res.stage == galaxy::GameStage::RUNNING){
		res.dump(std::cerr);

		galaxy::CommandListBuilder cmds;
    for (int i = 0; i < (int)res.state.ships.size(); i++) {
      const auto& sac = res.state.ships[i];
      const auto& ship = sac.ship;
      if (ship.role != res.static_info.self_role) { continue; }
      if (ship.params.x0 <= 50) { continue; }  // TODO

      // 取り出し
      if (ship_state_dic.find(i) == ship_state_dic.end()) {
        ship_state_dic[i] = ShipKidouState();
      }
      ShipKidouState& ship_state = ship_state_dic[i];
      long& kidou_idx = ship_state.kidou_idx;
      auto& kidou_plan_accel = ship_state.kidou_plan_accel;
      auto& is_on_kidou = ship_state.is_on_kidou;

      long galaxy_radius = res.static_info.galaxy_radius;
      long rem_turn = res.static_info.time_limit - res.state.elapsed;

      if (kidou_idx >= 0) {
        if (kidou_idx < (int)kidou_plan_accel.size()) {
          cmds.accel(ship.id, kidou_plan_accel[kidou_idx++]);
        }
      }
      else if (is_on_kidou || abs(ship.pos.x) <= near_galaxy_dist || abs(ship.pos.y) <= near_galaxy_dist) {
        // n手シミュレートする
        SimulateN sim(rem_turn, galaxy_radius, 3);
        sim.simulate_n(ship.pos, ship.vel, 0);
        if (sim.dead_turn == rem_turn) { // 死ぬまでのターンが返ってくるので最後まで活きていたら
          kidou_plan_accel = sim.ans;
          kidou_idx = 0;
          cmds.accel(ship.id, kidou_plan_accel[kidou_idx++]);
        }
        else {
          // なんとなく動かす（惑星付近にいる時に端に逃げるように）
          cmds.accel(ship.id, escape_from_galaxy(ship, near_galaxy_dist));
          is_on_kidou = true;
        }
      }
      else {
        // X0 または Y0方向に低速度で近づける
        cmds.accel(ship.id, toX0orY0(ship, res.static_info));
      }
    }
		res = ctx.command(cmds);
		++counter;
	}

	galaxy::global_finalize();
	return 0;
}
