#ifndef LIBGALAXY_GALAXY_HPP
#define LIBGALAXY_GALAXY_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include <cassert>

#include <curl/curl.h>


namespace galaxy {

//----------------------------------------------------------------------------
// Global initialization / finalization
//----------------------------------------------------------------------------
static void global_initialize(){
	curl_global_init(CURL_GLOBAL_ALL);
}

static void global_finalize(){
	curl_global_cleanup();
}


//----------------------------------------------------------------------------
// Utility functions
//----------------------------------------------------------------------------
inline bool is_numeric_string(const std::string& s){
	auto it = s.begin();
	if(*it == '-'){ ++it; }
	for(; it != s.end(); ++it){
		if(!std::isdigit(*it)){ return false; }
	}
	return true;
}


//----------------------------------------------------------------------------
// Low-level API
//----------------------------------------------------------------------------
struct Vec {
	long x, y;
	Vec() : x(0), y(0) { }
	Vec(long x, long y) : x(x), y(y) { }
};


class Element {

private:
	enum class ElementKind {
		NIL    = 0,
		NUMBER = 1,
		VECTOR = 2,
		LIST   = 3,
	};

	ElementKind          m_kind;
	long                 m_number;
	Vec                  m_vector;
	std::vector<Element> m_list;

public:
	Element()
		: m_kind(ElementKind::NIL)
		, m_number(0)
		, m_vector()
		, m_list()
	{ }

	Element(long x)
		: m_kind(ElementKind::NUMBER)
		, m_number(x)
		, m_vector()
		, m_list()
	{ }

	Element(Vec v)
		: m_kind(ElementKind::VECTOR)
		, m_number(0)
		, m_vector(std::move(v))
		, m_list()
	{ }

	Element(std::vector<Element> l)
		: m_kind(ElementKind::LIST)
		, m_number(0)
		, m_vector()
		, m_list(std::move(l))
	{ }

	bool is_nil() const { return m_kind == ElementKind::NIL; }

	bool is_number() const { return m_kind == ElementKind::NUMBER; }
	long as_number() const { return m_number; }

	bool is_vector() const { return m_kind == ElementKind::VECTOR; }
	const Vec& as_vector() const { return m_vector; }

	bool is_list() const { return m_kind == ElementKind::LIST; }
	const std::vector<Element>& as_list() const { return m_list; }

};


namespace detail {

static void serialize_recur(std::ostream& os, const Element& e){
	if(e.is_nil()){
		os << "nil ";
	}else if(e.is_number()){
		os << e.as_number() << " ";
	}else if(e.is_vector()){
		const auto& v = e.as_vector();
		os << "ap ap cons " << v.x << " " << v.y << " ";
	}else if(e.is_list()){
		bool is_first = true;
		os << "( ";
		for(const auto& c : e.as_list()){
			if(!is_first){ os << ", "; }
			is_first = false;
			serialize_recur(os, c);
		}
		os << ") ";
	}
}

}

static std::string serialize(const Element& e){
	std::ostringstream oss;
	detail::serialize_recur(oss, e);
	return oss.str();
}

namespace detail {

static Element deserialize_recur(std::istream& is){
	std::string token;
	is >> token;
	if(token == "nil"){
		return Element();
	}else if(is_numeric_string(token)){
		return Element(std::stol(token));
	}else if(token == "ap"){
		is >> token;  assert(token == "ap");
		is >> token;  assert(token == "cons");
		long x, y;
		is >> x >> y;
		return Element(Vec(x, y));
	}else if(token == "("){
		while(std::isspace(is.peek())){ is.ignore(); }
		if(is.peek() == ')'){
			is.ignore();
			return Element();
		}
		std::vector<Element> v;
		do {
			v.push_back(deserialize_recur(is));
			is >> token;
		} while(token != ")");
		return Element(std::move(v));
	}else{
		return Element();
	}
}

}

static Element deserialize(const std::string& s){
	std::istringstream iss(s);
	return detail::deserialize_recur(iss);
}


namespace detail {

static void modulate_recur(std::ostream& os, const Element& e){
	if(e.is_nil()){
		os << "00";
	}else if(e.is_number()){
		const long x = e.as_number();
		if(x == 0){
			os << "010";
		}else{
			const long y = std::abs(x);
			const long b = (sizeof(y) * 8 - __builtin_clzl(y) + 3) & ~3;
			os << (x > 0 ? "01" : "10");
			for(long i = 0; i < b; i += 4){ os << "1"; }
			os << "0";
			for(long i = b - 1; i >= 0; --i){ os << ((y >> i) & 1); }
		}
	}else if(e.is_vector()){
		const auto& v = e.as_vector();
		os << "11";
		modulate_recur(os, Element(v.x));
		modulate_recur(os, Element(v.y));
	}else if(e.is_list()){
		for(const auto& c : e.as_list()){
			os << "11";
			modulate_recur(os, c);
		}
		os << "00";
	}
}

}

static std::string modulate(const Element& e){
	std::ostringstream oss;
	detail::modulate_recur(oss, e);
	return oss.str();
}

namespace detail {

static Element demodulate_recur(std::istream& is){
	const char m0 = is.get();
	const char m1 = is.get();
	if(m0 == '0' && m1 == '0'){
		return Element();
	}else if(m0 == '1' && m1 == '1'){
		auto e0 = demodulate_recur(is);
		auto e1 = demodulate_recur(is);
		if(e1.is_nil()){
			std::vector<Element> v = { e0 };
			return Element(std::move(v));
		}else if(e0.is_number() && e1.is_number()){
			return Element(Vec(e0.as_number(), e1.as_number()));
		}else if(e1.is_list()){
			std::vector<Element> v = { e0 };
			for(auto& e : e1.as_list()){ v.push_back(std::move(e)); }
			return Element(v);
		}else{
			throw std::runtime_error("demodulation error");
		}
	}else{
		const long sign = (m0 == '0' ? 1 : -1);
		long b = 0, v = 0;
		while(is.get() == '1'){ b += 4; }
		for(long i = 0; i < b; ++i){ v = (v << 1) | (is.get() - '0'); }
		return Element(sign * v);
	}
}

}

static Element demodulate(const std::string& s){
	std::istringstream iss(s);
	return detail::demodulate_recur(iss);
}


//----------------------------------------------------------------------------
// Structures
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
// Context
//----------------------------------------------------------------------------
class GalaxyContext {

private:
	const long m_player_key;
	const std::string m_endpoint;

public:
	GalaxyContext()
		: m_player_key(0)
		, m_endpoint()
	{ }

	GalaxyContext(long player_key, std::string endpoint)
		: m_player_key(player_key)
		, m_endpoint(std::move(endpoint))
	{ }

};

}

#endif
