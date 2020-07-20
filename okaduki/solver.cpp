#include <tuple>
#include <map>
#include <cmath>

#include "galaxy.hpp"


//----------------------------------------------------------------------------
// Settings
//----------------------------------------------------------------------------
const int UNIVERSE_CHECK_ITERATIONS = 15;
const double RELATIVE_ANGLE_THRESHOLD = 0.2;


//----------------------------------------------------------------------------
// Utilities
//----------------------------------------------------------------------------
using Vec = galaxy::Vec;

uint32_t xor64(void) {
  static uint64_t x = 88172645463325252ULL;
  x = x ^ (x << 13); x = x ^ (x >> 7);
  return x = x ^ (x << 17);
}

int32_t random(int32_t a, int32_t b) {
	uint32_t x = xor64() % (b - a);
	return x + a;
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

void dump_vec(const Vec& vec, std::ostream& os) {
	os << "(" << vec.x << ", " << vec.y << ")" << std::endl;
}

bool universe_check(const Vec& p0, const Vec& d0, int n, long fld_r, long planet_r = -1){
	Vec p = p0, d = d0;
	for(int i = 0; i < n; ++i){
		const auto next = simulate(p, d);
		if(std::abs(next.first.x) > fld_r){ return false; }
		if(std::abs(next.first.y) > fld_r){ return false; }

		if(std::abs(next.first.x) <= planet_r
			&& std::abs(next.first.y) <= planet_r){ return false; }
		p = next.first;
		d = next.second;
	}
	return true;
}

int universe_check_step(const Vec& p0, const Vec& d0, long fld_r, long planet_r = -1){
	const int MAX = 256;
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

Vec compute_accel(const Vec& p, const Vec& d, long fld_radius){
	const Vec dd(p.x >= 0 ? -1 : 1, p.y >= 0 ? -1 : 1);
	const auto next = simulate(p, Vec(d.x - dd.x, d.y - dd.y));
	if(universe_check(next.first, next.second, UNIVERSE_CHECK_ITERATIONS, fld_radius, -1)){
		return dd;
	}else{
		return Vec();
	}
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

Vec random_accel(const Vec& p, const Vec& d, long fld_radius, long planet_radius){
	const long abs_x = std::abs(p.x), abs_y = std::abs(p.y);
	Vec dd;
	if (abs_x < abs_y) {
		dd.y = random(0, 2) * 2 - 1;
		dd.x = 0;
	}
	else {
		dd.x = random(0, 2) * 2 - 1;
		dd.y = 0;
	}

	const auto next = simulate(p, Vec(d.x - dd.x, d.y - dd.y));
	if(universe_check(next.first, next.second, UNIVERSE_CHECK_ITERATIONS, fld_radius, planet_radius)){
		return dd;
	}else{
		return Vec();
	}
}

// pos, vel
using Cache = std::tuple<long,long,long,long>;
void precalc_orbit(long fld_r, long pln_r, std::map<Cache,long>& res) {
	const long VEL_SIZE = 10;

	for (long y = -fld_r; y <= fld_r; ++y) {
		for (long x = -fld_r; x <= fld_r; ++x) {
			for (long vy = -VEL_SIZE; vy <= VEL_SIZE; ++vy) {
				for (long vx = -VEL_SIZE; vx <= VEL_SIZE; ++vx) {
					Vec pos(x, y);
					Vec vel(vx, vy);
					
					auto idx = std::make_tuple(x, y, vx, vy);
					res[idx] = universe_check_step(pos, vel, fld_r, pln_r);
				}
			}
		}
	}
}

long get_orbit_step(const std::map<Cache,long>& cache, const Vec& pos, const Vec& vel) {
	auto idx = std::make_tuple(pos.x, pos.y, vel.x, vel.y);
	auto it = cache.find(idx);

	if (it != std::end(cache))
		return it->second;
	return -1;
}

bool find_neighbor_orbit(
	const std::map<Cache,long>& memo
	, const Vec& pos
	, const Vec& vel
	, long time
	, Vec& res
) {
	for (long vy = -1; vy <= 1; ++vy) {
		for (long vx = -1; vx <= 1; ++vx) {
			const auto next = simulate(pos, Vec(vel.x - vx, vel.y - vy));
			long step = get_orbit_step(memo, next.first, next.second);

			if (step >= time) {
				res.x = vx;
				res.y = vy;
				return true;
			}
		}	
	}
	return false;
}

//----------------------------------------------------------------------------
// Defender
//----------------------------------------------------------------------------
void defender(
	galaxy::GalaxyContext& ctx,
	const galaxy::GameResponse& init_res)
{
	galaxy::GameResponse res;

	// Start
	galaxy::ShipParams ship_params;
	ship_params.x1 =  0;
	ship_params.x2 = 10;
	ship_params.x3 =  1;
	ship_params.x0 =
		  init_res.static_info.parameter_capacity
		-  4 * ship_params.x1
		- 12 * ship_params.x2
		-  2 * ship_params.x3;
	res = ctx.start(ship_params);

	std::map<Cache,long> memo;
	precalc_orbit(res.static_info.universe_radius, res.static_info.galaxy_radius, memo);
	std::map<long, bool> on_orbit;

	// Command loop
	while(res.stage == galaxy::GameStage::RUNNING){
		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role != res.static_info.self_role){ continue; }

			if (!on_orbit.count(ship.id)) on_orbit[ship.id] = false;
			if (on_orbit[ship.id]) continue;

			long rest_time = res.static_info.time_limit - res.state.elapsed + 1;
			Vec accel;
			bool found = find_neighbor_orbit(memo, ship.pos, ship.vel, rest_time, accel);
			if (!found) {
				accel = compute_accel_far_away(
					ship.pos, ship.vel, res.static_info.universe_radius);
			}
			else {
				on_orbit[ship.id] = true;
			}

			if(accel.x != 0 || accel.y != 0){
				cmds.accel(ship.id, accel);
			}
		}
		res = ctx.command(cmds);
	}
}


//----------------------------------------------------------------------------
// Attacker
//----------------------------------------------------------------------------
double check_relative_angle(const Vec& a, const Vec& b){
	if(a == b){ return true; }
	const auto rad = std::atan2(static_cast<double>(b.y - a.y), static_cast<double>(b.x - a.x));
	return std::cos(rad * 8.0) > RELATIVE_ANGLE_THRESHOLD;
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

void attacker(
	galaxy::GalaxyContext& ctx,
	const galaxy::GameResponse& init_res)
{
	galaxy::GameResponse res;

	// Start
	galaxy::ShipParams ship_params;
	ship_params.x1 = 64;
	ship_params.x2 = 10;
	ship_params.x3 =  1;
	ship_params.x0 =
		  init_res.static_info.parameter_capacity
		-  4 * ship_params.x1
		- 12 * ship_params.x2
		-  2 * ship_params.x3;
	res = ctx.start(ship_params);

	// Command loop
	const long bound_radius = std::max(
		res.static_info.universe_radius / 3,
		res.static_info.galaxy_radius + 16);
	HistoricalPredictor historical_predictor;
	InertialPredictor inertial_predictor;

	while(res.stage == galaxy::GameStage::RUNNING){
		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role == res.static_info.self_role){
				// Acceleration
				const auto accel = compute_accel(ship.pos, ship.vel, bound_radius);
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
		}
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role != res.static_info.self_role){
				historical_predictor.update(ship);
				inertial_predictor.update(ship);
			}
		}
		res = ctx.command(cmds);
	}
}


//----------------------------------------------------------------------------
// Entry
//----------------------------------------------------------------------------
int main(int argc, char *argv[]){
	if(argc < 3){
		std::cerr << "Usage: " << argv[0] << " endpoint player_key" << std::endl;
		return 0;
	}

	galaxy::global_initialize();

	const std::string endpoint = argv[1];
	const long player_key = atol(argv[2]);

	galaxy::GalaxyContext ctx(endpoint, player_key);
	const galaxy::GameResponse init_res = ctx.join();

	if(init_res.static_info.self_role == galaxy::PlayerRole::ATTACKER){
		attacker(ctx, init_res);
	}else{
		defender(ctx, init_res);
	}

	galaxy::global_finalize();
	return 0;
}