#include <tuple>
#include <map>

#include "galaxy.hpp"


//----------------------------------------------------------------------------
// Settings
//----------------------------------------------------------------------------
const int UNIVERSE_CHECK_ITERATIONS = 15;


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

	// Command loop
	while(res.stage == galaxy::GameStage::RUNNING){
		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role != res.static_info.self_role){ continue; }
			const auto accel = compute_accel(
				ship.pos, ship.vel, res.static_info.universe_radius);
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
	const long bound_radius = res.static_info.universe_radius / 2;
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
						if(prediction.first){
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
