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

void guruguru() {
  
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
	ship_params.x0 = (self_role == galaxy::PlayerRole::ATTACKER ? 510 - 40 : 446) - 120;  // TODO
	ship_params.x1 = (self_role == galaxy::PlayerRole::ATTACKER ? 10 : 0);
	ship_params.x2 = 10;
	ship_params.x3 = 1;
	res = ctx.start(ship_params);

	// Command loop
	
	long counter = 0;
	while(res.stage == galaxy::GameStage::RUNNING){
		res.dump(std::cerr);

		galaxy::CommandListBuilder cmds;
    galaxy::CommandListBuilder cmds2;
    int eneId = -1;
    int myId = -1;
    bool attack = false;
    for (int i = 0; i < res.state.ships.size(); i++) {
      const auto& sac = res.state.ships[i];
      const auto& ship = sac.ship;
      if (ship.role != res.static_info.self_role) { eneId = i; continue; }
      else myId = i;
      if(ship.params.x0 <= 50){ continue; }  // TODO

      long distx = std::min(abs(ship.pos.x - res.static_info.galaxy_radius), abs(ship.pos.x + res.static_info.galaxy_radius));
      long disty = std::min(abs(ship.pos.y - res.static_info.galaxy_radius), abs(ship.pos.y + res.static_info.galaxy_radius));
      long dist = std::min(distx, disty);
      long r = res.static_info.galaxy_radius;
      long x = ship.pos.x;
      long y = ship.pos.y;

      long dx, dy;
      bool iny, inx;
      inx = (-r <= x && x <= r);
      iny = (-r <= y && y <= r);

      if (x <= -r && iny) dx = 1, dy = -1;
      if (x <= -r && y <= r) dx = 1, dy = -1;

      if (x <= -r && y >= r) dx = -1, dy = -1;
      if (inx && y >= r) dx = -1, dy = -1;

      if (x >= r && y >= r) dx = -1, dy = 1;
      if (x >= r && iny) dx = -1, dy = 1;
      
      if (x >= r && y <= r) dx = 1, dy = 1;
      if (inx && y <= r) dx = 1, dy = 1;

      if (abs(ship.vel.x) > 4) {
        dx = (ship.vel.x > 0 ? 1 : -1);
      }
      if (abs(x) > r * 5) {
        dx = 0;
      }
      if (abs(ship.vel.y) > 4) {
        dy = (ship.vel.y > 0 ? 1 : -1);
      }
      if (abs(y) > r * 5) {
        dy = 0;
      }

      cmds.accel(ship.id, galaxy::Vec(dx, dy));
    }
    if (myId != -1) {
        const auto& myship = res.state.ships[myId].ship;
        if (eneId != -1 && myship.role == galaxy::PlayerRole::ATTACKER) {
        const auto& opship = res.state.ships[eneId].ship;

        double distEne = std::sqrt(double((myship.pos.x - opship.pos.x) * (myship.pos.x - opship.pos.x) + (myship.pos.y - opship.pos.y) * (myship.pos.y - opship.pos.y)));
        if (distEne < 256) {
          attack = true;
          cmds2.shoot(myship.id, opship.pos);
        }
      }
    }
    if (attack)
      res = ctx.command(cmds2);
    else 
		  res = ctx.command(cmds);
		++counter;
	}

	galaxy::global_finalize();
	return 0;
}
