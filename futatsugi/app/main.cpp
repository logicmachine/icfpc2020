// ICFPC2020 galaxy solver by foota
// https://icfpc2020-api.testkontur.ru/aliens/send
// b0a3d915b8d742a39897ab4dab931721

#include <iostream>
#include <regex>
#include <string>
#include <vector>
#include <cmath>
#include <random>
#include <queue>

#include "galaxy.hpp"

const int UNIVERSE_CHECK_ITERATIONS = 15;
const double RELATIVE_ANGLE_THRESHOLD = 0.2;

std::random_device seed_gen;
std::mt19937 engine(seed_gen());

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

Vec compute_accel(const Vec& p, const Vec& d, long radius){
	const Vec dd(p.x >= 0 ? -1 : 1, p.y >= 0 ? -1 : 1);
	const auto next = simulate(p, Vec(d.x - dd.x, d.y - dd.y));
	if(universe_check(next.first, next.second, UNIVERSE_CHECK_ITERATIONS, radius)){
		return dd;
	}else{
		return Vec();
	}
}
#if 0
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
#endif
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

#if 1
double check_relative_angle(const Vec& a, const Vec& b){
	if(a == b){ return true; }
	const auto dx = std::abs(b.x - a.x);
	const auto dy = std::abs(b.y - a.y);
	return std::min(std::min(dx, dy), std::abs(dx - dy)) < RELATIVE_ANGLE_THRESHOLD;
}

class HistoricalPredictor {

private:
	using ship_state_t = std::pair<Vec, Vec>;
	std::map<long, ship_state_t> m_last_ship_states;
	std::map<ship_state_t, Vec> m_predictions;

public:
	HistoricalPredictor()
		: m_last_ship_states()
		, m_predictions()
	{ }

	void update(const galaxy::ShipState& ship){
		const ship_state_t cur_state(ship.pos, ship.vel);
		const auto it = m_last_ship_states.find(ship.id);
		if(it != m_last_ship_states.end()){
			const auto prev_state = it->second;
			m_predictions[prev_state] = ship.pos;
		}
		m_last_ship_states[ship.id] = cur_state;
	}

	std::pair<bool, Vec> predict(const galaxy::ShipState& ship){
		const ship_state_t cur_state(ship.pos, ship.vel);
		const auto it = m_predictions.find(cur_state);
		if(it != m_predictions.end()){
			return std::make_pair(true, it->second);
		}else{
			return std::make_pair(false, Vec());
		}
	}

};

class InertialPredictor {

private:
	using ship_state_t = std::pair<Vec, Vec>;
	std::map<long, ship_state_t> m_last_ship_states;
	std::map<long, std::pair<long, long>> m_inertial_rate;

public:
	InertialPredictor()
		: m_last_ship_states()
		, m_inertial_rate()
	{ }

	void update(const galaxy::ShipState& ship){
		const ship_state_t cur_state(ship.pos, ship.vel);
		const auto it = m_last_ship_states.find(ship.id);
		if(it != m_last_ship_states.end()){
			const auto prev_state = it->second;
			const auto sim = simulate(prev_state.first, prev_state.second);
			auto& rate = m_inertial_rate[ship.id];
			rate.first  += (sim.first == cur_state.first);
			rate.second += 1;
		}else{
			m_inertial_rate[ship.id] = std::pair<long, long>(0, 0);
		}
		m_last_ship_states[ship.id] = cur_state;
	}

	std::pair<bool, Vec> predict(const galaxy::ShipState& ship){
		const auto it = m_inertial_rate.find(ship.id);
		if(it != m_inertial_rate.end()){
			const auto& rate = it->second;
			if(rate.second >= 5 && 100 * rate.first / rate.second > 75){
				const auto s = simulate(ship.pos, ship.vel);
				return std::make_pair(true, s.first);
			}
		}
		return std::make_pair(false, Vec());
	}

};
#endif

int universe_check_step(const Vec& p0, const Vec& d0, long fld_r, long planet_r = -1){
	const int MAX = 384;
	Vec p = p0, d = d0;
	for(int i = 0; i < MAX; ++i){
		const auto next = simulate(p, d);
		if(std::abs(next.first.x) > fld_r){ return i; }
		if(std::abs(next.first.y) > fld_r){ return i; }

		if(std::abs(next.first.x) <= planet_r
			&& std::abs(next.first.y) <= planet_r){ return i; }
		p = next.first;
		d = next.second;
	}
	return MAX;
}

Vec compute_accel_far_away(const Vec& p, const Vec& d, long fld_radius){
	const Vec dd(p.x >= 0 ? -1 : 1, p.y >= 0 ? -1 : 1);
	const auto next = simulate(p, Vec(d.x - dd.x, d.y - dd.y));
	if(universe_check(next.first, next.second, UNIVERSE_CHECK_ITERATIONS, fld_radius, -1)){
		return dd;
	}else{
		return Vec();
	}
}

long get_orbit_step(Vec pos, Vec vel, long fld_r, long pln_r) {
	long step = universe_check_step(pos, vel, fld_r, pln_r);
	return step;
}

bool find_neighbor_orbit(
	const Vec& pos_
	, const Vec& vel_
	, long time_
	, Vec& res
	, int step_
	, long fld_r
	, long pln_r
) {
	// vx, vy, time, step, pos, vel
	using State = std::tuple<long, long, long, long, Vec, Vec>;
	std::queue<State> q;

	q.push(std::make_tuple(0, 0, time_, step_, pos_, vel_));
	while(!q.empty()) {
		auto state = q.front();
		q.pop();

		long vx, vy, time, step;
		Vec pos, vel;
		std::tie(vx, vy, time, step, pos, vel) = state;

		if (step == 0) continue;

		for (long n_vy = -1; n_vy <= 1; ++n_vy) {
			for (long n_vx = -1; n_vx <= 1; ++n_vx) {
				long first_vx = step == step_ ? n_vx : vx;
				long first_vy = step == step_ ? n_vy : vy;

				const auto next = simulate(pos, Vec(vel.x - n_vx, vel.y - n_vy));
				long alive_turn = get_orbit_step(next.first, next.second, fld_r, pln_r);

				if (alive_turn >= time) {
					res.x = first_vx;
					res.y = first_vy;
					return true;
				}

				q.push(std::make_tuple(
					first_vx, first_vy,
					time-1, step-1, next.first, next.second
				));
			}
		}
	}
	return false;
}

class GalaxySolver {
private:
	const std::string endpoint;
	const long player_key;
	galaxy::GalaxyContext ctx;
	galaxy::GameResponse res;
	galaxy::PlayerRole self_role;
	//galaxy::CommandListBuilder cmds;
	
	bool is_attacker = false;
	std::vector<int> join_params;
	galaxy::ShipParams ship_params;
	int capacity = 0;
	int galaxy_radius = 0;
	int universe_radius = 0;

	std::vector<Vec> TARGETS;

#if 1
	HistoricalPredictor historical_predictor;
	InertialPredictor inertial_predictor;
#endif

public:
	GalaxySolver(const std::string& endpoint, const long player_key) : 
		endpoint(endpoint), 
		player_key(player_key),
		ctx(endpoint, player_key) {
		galaxy::global_initialize();

		TARGETS.push_back(Vec(0, 0));
		TARGETS.push_back(Vec(1, 0));
		TARGETS.push_back(Vec(0, -1));
		TARGETS.push_back(Vec(-1, 0));
	}
	
	virtual ~GalaxySolver() {
		galaxy::global_finalize();
	}

#if 0
	std::vector<int> get_join_parameters() {
		//const std::string& raw_send = ctx.m_raw_send;
		const std::string& raw_recv = ctx.m_raw_recv;

		std::stringstream ss;
		std::vector<int> params;

		// "( 1 , 0 , ( 256 , 1 , ( 448 , 1 , 64 ) , ( 16 , 128 ) , nil ) , nil )"
		ss << raw_recv;
		char c = 0;
		int n = 0;
		int p1 = -1;
		int p2 = -1;
		int p3 = -1;
		int p4 = -1;
		int p5 = -1;
		ss >> c >> n >> c >> n >> c >> c >> n >> c >> n >> c >> c >> p1 >> c >> p2 >> c >> p3 >> c >> c >> c >> p4 >> c >> p5;
		std::cerr << "(" << p1 << ", " << p2 << ", " << p3 << "), (" << p4 << ", " << p5 << ")" << std::endl; /// debug
		params.push_back(p1);
		params.push_back(p2);
		params.push_back(p3);
		params.push_back(p4);
		params.push_back(p5);
		
		return params;
	}

	std::vector<int> get_start_parameters() {
		//const std::string& raw_send = ctx.m_raw_send;
		//const std::string& raw_recv = ctx.m_raw_recv;
		std::vector<int> params;
		return params;
	}
#endif

	int orbital_check(const Vec& pos, const Vec& vel) {
		int n;
		Vec p(pos);
		Vec v(vel);
		for (n = 0; n < 300; n++) {
			const Vec g = gravity(p);
			v.x += g.x;
			v.y += g.y;
			p.x += v.x;
			p.y += v.y;
			if (abs(p.x) <= galaxy_radius || abs(p.y) <= galaxy_radius) return n;
			if (abs(p.x) >= universe_radius || abs(p.y) >= universe_radius) return n;
		}
		return n;
	}

#if 0
	int orbital_search(const Vec& pos, const Vec& vel, int current, int depth) {
		if (depth > 3) return current;
		for (const Vec& target : TARGETS) {
			Vec new_pos(pos.x + vel.x, pos.x + vel.y);
			Vec new_vel(vel.x + target.x, vel.x, target.y);
			int next = orbital_check(new_pos, new_vel);
			if (next > current) {
				return orbital_search(new_pos, new_vel, next, depth + 1);
			}
		}
	}
#endif

	const galaxy::GameStage stage() {
		return res.stage;
	}

	double random() {
		std::uniform_real_distribution<> dist(0.0, 1.0);
		return dist(engine);
	}
	void dump() {
		res.dump(std::cerr);
	}

	void join() {
		res = ctx.join();
		//std::cerr << ctx.m_raw_send << std::endl;  /// debug
		//std::cerr << ctx.m_raw_recv << std::endl;  /// debug
		res.dump(std::cerr);  /// debug
		self_role = res.static_info.self_role;
		is_attacker = (self_role == galaxy::PlayerRole::ATTACKER);
	}

	void start() {
		/*
		-  4 * ship_params.x1
        - 12 * ship_params.x2
        -  2 * ship_params.x3;
		*/
		//join_params = get_join_parameters();
		//galaxy_radius = join_params[3];
		//universe_radius = join_params[4];
		galaxy_radius = res.static_info.galaxy_radius;
		universe_radius = res.static_info.universe_radius;
		capacity = res.static_info.parameter_capacity;
		ship_params.x1 = (is_attacker ? 64 : 0);  // laser power (?)
		ship_params.x2 = (is_attacker ? 10 : 5);  // detonation power (?)
		ship_params.x3 = (is_attacker ? 1 : 128);  // 
		ship_params.x0 = capacity - (ship_params.x1 * 4 + ship_params.x2 * 12 + ship_params.x3 * 2);
#if 0
		ship_params.x0 = (self_role == galaxy::PlayerRole::ATTACKER ? 510 : 446);  // TODO
		ship_params.x1 = 0;
		ship_params.x2 = 0;
		ship_params.x3 = 1;
#endif
		std::cerr << ship_params.x0 << ", " << ship_params.x1 << ", " << ship_params.x2 << ", " << ship_params.x3 << std::endl;
		res = ctx.start(ship_params);
		
		//auto start_params = get_start_parameters();
	}

	void attacker(galaxy::CommandListBuilder& cmds) {
		const long bound_radius = std::max(
			res.static_info.universe_radius / 3,
			res.static_info.galaxy_radius + 16);
		
		for (const auto& sac : res.state.ships) {
#if 0
			const auto& ship = sac.ship;
			if (ship.role != res.static_info.self_role) { continue; }
			int sign_x = ship.pos.x < 0 ? -1 : 1;
			int sign_y = ship.pos.y < 0 ? -1 : 1;
			auto g = abs(ship.pos.x) > abs(ship.pos.y) ? Vec(-sign_x, 0) : Vec(0, -sign_y);
			cmds.accel(ship.id, g);
#else
			const auto& ship = sac.ship;
			if (ship.role == res.static_info.self_role) {
				// Acceleration
				const auto accel = compute_accel(
					ship.pos, ship.vel, bound_radius, res.static_info.galaxy_radius);
				if(accel.x != 0 || accel.y != 0){ cmds.accel(ship.id, accel); }
				// Shooting
				if(ship.x5 > 0){ continue; } // TODO
				for(const auto& target_sac : res.state.ships){
					const auto& target = target_sac.ship;
					if(target.role != res.static_info.self_role){
						std::pair<bool, Vec> prediction;
						prediction = historical_predictor.predict(target);
						if(!prediction.first){
							prediction = inertial_predictor.predict(target);
						}
						if(prediction.first && check_relative_angle(ship.pos, prediction.second)){
							cmds.shoot(ship.id, prediction.second, ship.params.x1);
						}
					}
				}
			}
#endif
		}
#if 1
		for (const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role != res.static_info.self_role){
				historical_predictor.update(ship);
				inertial_predictor.update(ship);
			}
		}
#endif
		res = ctx.command(cmds);
	}

	void defender(galaxy::CommandListBuilder& cmds) {
		static int cnt = 1;
		for (const auto& sac : res.state.ships) {
			const auto& ship = sac.ship;
#if 0
				if (ship.role != res.static_info.self_role) { continue; }
				//if (ship.params.x0 <= 100) { continue; }  // TODO
				//long ddx = ship.pos.x >= 0 ? -1 : 1;
				//long ddy = ship.pos.y >= 0 ? -1 : 1;
				long ddx = 0;
				long ddy = 0;
				if (ship.pos.x >= 0) ddx = -1;
				else if (ship.pos.x < 0) ddx = 1;
				if (ship.pos.y >= 0) ddx = -1;
				else if (ship.pos.y < 0) ddy = 1;
				if (ship.pos.x > galaxy_radius && ship.pos.x < universe_radius) ddx = -1;
				if (ship.pos.x > -galaxy_radius && ship.pos.x < -universe_radius) ddx = 1;
				if (ship.pos.y > galaxy_radius && ship.pos.y < universe_radius) ddy = -1;
				if (ship.pos.y > -galaxy_radius && ship.pos.y < -universe_radius) ddy = 1;
				cmds.accel(ship.id, Vec(ddx, ddy));
#endif
			if (cnt > 5 && ship.params.x3 > 1) {
				galaxy::ShipParams params;
				params.x0 = 0;
				params.x1 = 0;
				params.x2 = 0;
				params.x3 = 1;
				cmds.fork(ship.id, params);
			}
#if 0
#if 1
			const auto accel = compute_accel(
				ship.pos, ship.vel,
				static_cast<int>(res.static_info.universe_radius * 0.5),
				res.static_info.galaxy_radius);
#else
			const auto accel = compute_accel(ship.pos, ship.vel, res.static_info.universe_radius);
#endif
			if (accel.x != 0 || accel.y != 0) {
				cmds.accel(ship.id, accel);
			} else {
				if (abs(ship.pos.x) < res.static_info.universe_radius * 0.5 && abs(ship.pos.y) < res.static_info.universe_radius * 0.5) {
					if (abs(ship.pos.x) < abs(ship.pos.y)) {
						Vec ddx(0, ship.pos.y > 0 ? -1 : 1);
						cmds.accel(ship.id, ddx);
					} else {
						Vec ddx(ship.pos.x > 0 ? -1 : 1, 0);
						cmds.accel(ship.id, ddx);
					}
				}
			}
#endif
#if 0
			const auto accel = compute_accel(
				ship.pos, ship.vel,
				static_cast<int>(res.static_info.universe_radius * 0.5),
				res.static_info.galaxy_radius);
			if (accel.x != 0 || accel.y != 0) {
				cmds.accel(ship.id, accel);
			} else {
			double d = sqrt(static_cast<double>(ship.pos.x * ship.pos.x + ship.pos.y * ship.pos.y));
			double x = ship.pos.y / d;
			int dx = 0;
			int dy = 0;
			if (abs(x) > 0.9) {
				if (ship.pos.y > 0) {
					if (ship.vel.x < 5) dx = -1;
				} else {
					if (ship.vel.x > -5) dx = 1;
				}
				if (ship.vel.y > 2) dy = 1;
				if (ship.vel.y < -2) dy = -1;
			} else if (abs(x) < 0.1) {
				if (ship.pos.y > 0) {
					if (ship.vel.y < 5) dy = -1;
				} else {
					if (ship.vel.y > -5) dy = 1;
				}
				if (ship.vel.x > 2) dx = 1;
				if (ship.vel.x < -2) dx = -1;
			}
			if (dx != 0 || dy != 0) cmds.accel(ship.id, Vec(dx, dy));
			}
#endif
#if 0
			//std::uniform_real_distribution<> dist(0.0, 1.0);
			if (random() < 0.5) {
			const auto accel = compute_accel(
				ship.pos, ship.vel,
				static_cast<int>(res.static_info.universe_radius * 0.5),
				res.static_info.galaxy_radius);
				if (accel.x != 0 || accel.y != 0) cmds.accel(ship.id, accel);
			} else {
			Vec best_accel(0, 0);
			int m = 0;
			for (int x = -1; x <= 1; x++) {
				for (int y = -1; y <= 1; y++) {
					//int nsteps = orbital_search(ship.pos, ship.vel, Vec(x, y));
					int nsteps = orbital_check(ship.pos, Vec(ship.vel.x + x, ship.vel.y + y));
					//std::cerr << "orbital: " << nsteps << std::endl;
					if (nsteps > m) {
						m = nsteps;
						best_accel.x = -x;
						best_accel.y = -y;
					}
				}
			}
			if (best_accel.x != 0 || best_accel.y != 0) cmds.accel(ship.id, best_accel);
			}
#endif
			const long rest_time = res.static_info.time_limit - res.state.elapsed + 1;
			Vec accel;
			int step = 3;
			bool found = find_neighbor_orbit(ship.pos, ship.vel, rest_time, accel, step, res.static_info.universe_radius, res.static_info.galaxy_radius);
			if (!found || random() < 0.2) {
				accel = compute_accel(
					ship.pos, ship.vel,
					static_cast<int>(res.static_info.universe_radius * 0.7),
					res.static_info.galaxy_radius);
			}
			if (accel.x != 0 || accel.y != 0) cmds.accel(ship.id, accel);
		}
		cnt++;
	}

	void solve() {
		galaxy::CommandListBuilder cmds;
		if (is_attacker) attacker(cmds);
		else defender(cmds);
		res = ctx.command(cmds);
	}
};

int main(int argc, char* argv[])
{
	if (argc < 3) {
		std::cerr << "Usage: " << argv[0] << " endpoint player_key" << std::endl;
		return 1;
	}

	const std::string endpoint = argv[1];
	const long player_key = std::atol(argv[2]);

	GalaxySolver solver(endpoint, player_key);

	solver.join();
	solver.start();
	solver.dump();
	while (solver.stage() == galaxy::GameStage::RUNNING) {
		solver.solve();
		solver.dump();
	}

	std::cerr << solver.stage() << std::endl; /// debug

	return 0;
}
