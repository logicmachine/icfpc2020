// ICFPC2020 galaxy solver by foota
// https://icfpc2020-api.testkontur.ru/aliens/send
// b0a3d915b8d742a39897ab4dab931721

#include <iostream>
#include <regex>
#include <string>
#include <vector>

#include "galaxy.hpp"

class GalaxySolver {
private:
	const std::string endpoint;
	const long player_key;
	galaxy::GalaxyContext ctx;
	galaxy::GameResponse res;
	galaxy::PlayerRole self_role;
	galaxy::CommandListBuilder cmds;
	
	bool is_attacker = false;
	std::vector<int> join_params;
	galaxy::ShipParams ship_params;

public:
	GalaxySolver(const std::string& endpoint, const long player_key) : 
		endpoint(endpoint), 
		player_key(player_key),
		ctx(endpoint, player_key) {
		galaxy::global_initialize();
	}
	
	virtual ~GalaxySolver() {
		galaxy::global_finalize();
	}

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

	const galaxy::GameStage stage() {
		return res.stage;
	}

	void dump() {
		res.dump(std::cerr);
	}

	void join() {
		res = ctx.join();
		std::cerr << ctx.m_raw_send << std::endl;  /// debug
		std::cerr << ctx.m_raw_recv << std::endl;  /// debug
		res.dump(std::cerr);  /// debug
		self_role = res.static_info.self_role;
		is_attacker = (self_role == galaxy::PlayerRole::ATTACKER);
	}

	void start() {
		join_params = get_join_parameters();
		int bonus = join_params[4] > 0 ? join_params[0] - 2 : (is_attacker ? 510 : 446); // 2: magic number
		ship_params.x0 = static_cast<int>(bonus * (is_attacker ? 0.8 : 1.0));
		ship_params.x1 = static_cast<int>(bonus * (is_attacker ? 0.05 : 0.0));  // laser power (?)
		//ship_params.x2 = bonus - (ship_params.x0 + ship_params.x1);          // detonation power (?)
		ship_params.x2 = 0;          // detonation power (?)
#if 0
		ship_params.x0 = (self_role == galaxy::PlayerRole::ATTACKER ? 510 : 446);  // TODO
		ship_params.x1 = 0;
		ship_params.x2 = 0;
#endif
		ship_params.x3 = 1;
		std::cerr << ship_params.x0 << ", " << ship_params.x1 << ", " << ship_params.x2 << ", " << ship_params.x3 << std::endl;
		res = ctx.start(ship_params);
		
		//auto start_params = get_start_parameters();
	}

	void solve() {
		static long counter = 0;
		const long interval = 4;
		if (is_attacker) {
			for (const auto& sac : res.state.ships) {
				const auto& ship = sac.ship;
				if (ship.role != res.static_info.self_role) { continue; }
				int sign_x = ship.pos.x < 0 ? -1 : 1;
				int sign_y = ship.pos.y < 0 ? -1 : 1;
				auto g = abs(ship.pos.x) > abs(ship.pos.y) ? galaxy::Vec(-sign_x, 0) : galaxy::Vec(0, -sign_y);
				cmds.accel(ship.id, g);
			}
		} else {
			if ((counter + 1) % interval != 0) {
				for (const auto& sac : res.state.ships) {
					const auto& ship = sac.ship;
					if (ship.role != res.static_info.self_role) { continue; }
					if (ship.params.x0 <= 100) { continue; }  // TODO
					const long ddx = ship.pos.x >= 0 ? -1 : 1;
					const long ddy = ship.pos.y >= 0 ? -1 : 1;
					cmds.accel(ship.id, galaxy::Vec(ddx, ddy));
				}
			}
			counter++;
		}
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
	while (solver.stage() == galaxy::GameStage::RUNNING) {
		solver.dump();
		solver.solve();
	}

	std::cerr << solver.stage() << std::endl; /// debug

	return 0;
}
