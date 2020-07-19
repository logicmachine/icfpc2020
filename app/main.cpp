#include "galaxy.hpp"

#include <limits>
#include <cmath>

struct Rect {
	galaxy::Vec lower;
	galaxy::Vec upper;

	Rect(){}

	Rect(const galaxy::Vec &lower, const galaxy::Vec &upper) {
		this->lower = lower;
		this->upper = upper;
	}
};

struct Line {
	galaxy::Vec p1;
	galaxy::Vec p2;

	Line(const galaxy::Vec &p1, const galaxy::Vec &p2) {
		this->p1 = p1;
		this->p2 = p2;
	}
};

bool intersect(const Line &l1, const Line &l2) {

	const auto ax = l1.p1.x;
	const auto ay = l1.p1.y;
	const auto bx = l1.p2.x;
	const auto by = l1.p2.y;
	const auto cx = l2.p1.x;
	const auto cy = l2.p1.y;
	const auto dx = l2.p2.x;
	const auto dy = l2.p2.y;

	const auto ta = (cx - dx) * (ay - cy) + (cy - dy) * (cx - ax);
	const auto tb = (cx - dx) * (by - cy) + (cy - dy) * (cx - bx);
	const auto tc = (ax - bx) * (cy - ay) + (ay - by) * (ax - cx);
	const auto td = (ax - bx) * (dy - ay) + (ay - by) * (ax - dx);

    return tc * td < 0 && ta * tb < 0;
}

bool visible(const galaxy::Vec &self, const galaxy::Vec &opponent, const Rect &aubit) {

	const auto player_line = Line(self, opponent);

	const galaxy::Vec ul(aubit.lower.x, aubit.upper.y);
	const galaxy::Vec ur(aubit.upper.x, aubit.upper.y);
	const galaxy::Vec dr(aubit.upper.x, aubit.upper.y);
	const galaxy::Vec dl(aubit.lower.x, aubit.lower.y);
	
	const std::vector<Line> rect_lines = {
		Line(ul, ur), 
		Line(ur, dr),
		Line(dr, dl), 
		Line(dl, ul)	
	};

	for (auto line : rect_lines) {
		if (intersect(player_line, line)) {
			return false;
		}
	}
	return true;
}

galaxy::Vec accel(const galaxy::Vec &ship, const galaxy::Vec &aubit_lower, const galaxy::Vec &aubit_upper) {

	const int cand_ax = ship.x < aubit_lower.x ? 1 : -1;
	const int cand_ay = ship.y < aubit_lower.y ? 1 : -1;

	if (aubit_lower.y <= ship.y && ship.y <= aubit_upper.y) {
		return galaxy::Vec(cand_ax, 0);
	}
	if (aubit_lower.x <= ship.x && ship.x <= aubit_upper.x) {
		return galaxy::Vec(0, cand_ay);
	}

	const auto dx = std::min(abs(ship.x - aubit_lower.x), abs(ship.x - aubit_upper.x));
	const auto dy = std::min(abs(ship.y - aubit_lower.y), abs(ship.y - aubit_upper.y));

	if (dx > dy) {
		return galaxy::Vec(cand_ax, 0);
	} else {
		return galaxy::Vec(0, cand_ay);
	}
	// TODO: dx == dy?
}

galaxy::Vec next_dir(const galaxy::Vec &ship_pos, const galaxy::Vec &ship_vel) {

	// 中心を (0, 0) と仮定して、直行する位置で、現在の vel に近い方向へ向かう

	// TODO: 的に近づく AI を書く

	galaxy::Vec ret;
	double ip = std::numeric_limits<int>::max();
	int index = 0;

	const int dy[8] = {-1, -1, -1, 0, 1, 1, 1, 0};
	const int dx[8] = {-1, 0, 1, 1, 1, 0, -1, -1}; 

	for (int i = 0; i < 8; i++) {
		const double norm = std::sqrt(dy[i] * dy[i] + dx[i] * dx[i]);
		const auto cur_ip = std::abs(ship_pos.x * dx[i]  + ship_pos.y * dy[i]) / norm;
		if (ip > cur_ip) {
			ip = cur_ip;
			index = i;
		}
	}
	
	const int op_index = (index + 4) % 8;

	const int c1 = ship_vel.x * dx[index] + ship_vel.y * dy[index];
	const int c2 = ship_vel.x * dx[op_index] + ship_vel.y * dy[op_index];

	return c1 > c2 ? galaxy::Vec(dx[index], dy[index]) : galaxy::Vec(dx[op_index], dy[op_index]);	
}

galaxy::Vec next_position(const galaxy::Vec &ship_pos, const galaxy::Vec &ship_vel, const galaxy::Vec &aubit_lower, 
		const galaxy::Vec &aubit_upper) {
	const auto a = accel(ship_pos, aubit_lower, aubit_upper);
	return galaxy::Vec (
		ship_pos.x + ship_vel.x + a.x,
		ship_pos.y + ship_vel.y + a.y
	);
}

void move_attcker(galaxy::GalaxyContext &ctx, galaxy::GameResponse &res) {

	const auto self_role = res.static_info.self_role;

	// Start
	galaxy::ShipParams ship_params;
	ship_params.x0 = (self_role == galaxy::PlayerRole::ATTACKER ? 510 : 446);  // TODO
	ship_params.x1 = 0;
	ship_params.x2 = 0;
	ship_params.x3 = 1;
	res = ctx.start(ship_params);

	// TODO: setup appropreate parameter
	Rect aubit;
	galaxy::Vec aubit_lower(-7, -7);
	galaxy::Vec aubit_upper(7, 7);

	std::vector<std::pair<int, galaxy::ShipState>> show_info;

	// Command loop
	const long interval = 4;
	long counter = 0;
	while(res.stage == galaxy::GameStage::RUNNING){

		res.dump(std::cerr);
		galaxy::CommandListBuilder cmds;

		// setup information
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role != res.static_info.self_role){ 
				show_info.emplace_back(counter, sac.ship);
			}
		}

		// decide turn
		for(const auto& sac : res.state.ships){
			const auto& ship = sac.ship;
			if(ship.role == res.static_info.self_role){ 
				if(ship.params.x0 <= 100){ continue; }  // TODO

				const auto op = show_info.back().second;

				if (visible(ship.pos, op.pos, aubit)) {
					const auto pos = next_position(ship.pos, ship.vel, aubit.lower, aubit.upper);
					cmds.shoot(ship.id, pos);
				}

				if (counter % interval == 0) {
					const auto a = next_dir(ship.pos, ship.vel);
					cmds.accel(ship.id, a);
				}
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
