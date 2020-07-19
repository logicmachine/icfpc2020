#include "galaxy.hpp"

int main(){
	galaxy::global_initialize();

	std::string line;
	std::getline(std::cin, line);

	const auto des = galaxy::deserialize(line);
	const auto ser = galaxy::serialize(des);

	std::cout << ser << std::endl;

	galaxy::global_finalize();
	return 0;
}
