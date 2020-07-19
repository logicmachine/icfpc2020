#include "galaxy.hpp"

int main(int argc, char *argv[]){
	if(argc < 2){
		std::cerr << "Usage: " << argv[0] << " endpoint" << std::endl;
		return 0;
	}

	galaxy::global_initialize();

	galaxy::RemoteQueryEngine remote_query(argv[1]);

	std::string line;
	std::getline(std::cin, line);
	
	const auto des = galaxy::deserialize(line);
	const auto mod = galaxy::modulate(des);
	const auto res = remote_query(mod);
	const auto dem = galaxy::demodulate(res);
	const auto ser = galaxy::serialize(dem);

	std::cout << ser << std::endl;

	galaxy::global_finalize();
	return 0;
}
