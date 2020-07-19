#include "galaxy.hpp"

int main(){
	galaxy::global_initialize();

	std::string line;
	std::getline(std::cin, line);

	const auto des = galaxy::deserialize(line);
	const auto mod = galaxy::modulate(des);
	const auto dem = galaxy::demodulate(mod);
	const auto ser = galaxy::serialize(dem);

	std::cout << mod << std::endl;
	std::cout << ser << std::endl;

	galaxy::global_finalize();
	return 0;
}
