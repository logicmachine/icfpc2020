#include "galaxy.hpp"

int main(int argc, char *argv[]){
	if(argc < 2){
		std::cerr << "Usage: " << argv[0] << " endpoint" << std::endl;
		return 0;
	}

	galaxy::global_initialize();

	const std::string endpoint = argv[1];
	galaxy::GalaxyContext ctx(endpoint, 0);

	const auto ri = ctx.create_room();
	std::cout << "Attacker: " << ri.attacker_key << std::endl;
	std::cout << "Defender: " << ri.defender_key << std::endl;

	galaxy::global_finalize();
	return 0;
}
