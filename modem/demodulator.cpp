#include <iostream>
#include <string>
#include <vector>
#include <memory>

enum class Kind {
	NIL    = 0,
	NUMBER = 1,
	PAIR   = 2
};

struct Element {
	Kind kind;
	long long int value;
	std::unique_ptr<Element> x0;
	std::unique_ptr<Element> x1;

	Element() : kind(Kind::NIL) { }
	Element(long long int x) : kind(Kind::NUMBER), value(x) { }
};


std::ostream& operator<<(std::ostream& os, const Element& e){
	if(e.kind == Kind::NIL){
		os << "nil";
	}else if(e.kind == Kind::NUMBER){
		os << e.value;
	}else if(e.kind == Kind::PAIR){
		os << "(" << *(e.x0) << ", " << *(e.x1) << ")";
	}
	return os;
}


Element decode(const char *&p){
	if(p[0] == '0' && p[1] == '0'){
		p += 2;
		return Element();
	}else if(p[0] == '1' && p[1] == '1'){
		p += 2;
		Element e;
		e.kind = Kind::PAIR;
		e.x0 = std::make_unique<Element>(decode(p));
		e.x1 = std::make_unique<Element>(decode(p));
		return e;
	}else{
		const int sign = (p[0] == '0' ? 1 : -1);
		p += 2;
		int depth = 0;
		while(*p != '0'){
			depth += 4;
			++p;
		}
		++p;
		long long int value = 0;
        fprintf(stderr, "depth = %d\n", depth);
		for(int i = 0; i < depth; ++i){
			value = (value << 1) | (*p - '0');
            fprintf(stderr, "value = %lld\n", value);
			++p;
		}
		return Element(value * sign);
	}
}


int main(int argc, char *argv[]){
	std::string s;
	std::cin >> s;
	const char *ptr = s.c_str();
	std::cout << decode(ptr) << std::endl;
	return 0;
}

