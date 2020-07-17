// alien's number generator
// g++ alien_number.cpp -o alien_number -O2 -Wall

#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>

int main(int argc, char* argv[])
{
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " number" << std::endl;
		exit(1);
	}

	long long n = std::atoll(argv[1]);
	std::string sign = n < 0 ? "10" : "01";
	std::stringstream ss;

	n = std::abs(n);
	for (int i = 0; n || i % 4 != 0; i++) {
		ss << (n & 1);
		n >>= 1;
	}
	std::string number(ss.str());
	std::reverse(number.begin(), number.end());
	int len = ss.str().length();
	std::cout << sign;
	for (int i = 0; i < len / 4; i++) std::cout << 1;
	std::cout << 0 << number << std::endl;

	return 0;
}
