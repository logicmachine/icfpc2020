#ifndef LIBGALAXY_GALAXY_HPP
#define LIBGALAXY_GALAXY_HPP

#include <iostream>
#include <sstream>
#include <array>
#include <string>
#include <vector>
#include <memory>

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
// Networking
//----------------------------------------------------------------------------
class RemoteQueryEngine {

private:
	std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> m_curl;
	std::string m_endpoint;

	static size_t callback(char *buffer, size_t size, size_t nmemb, void *userdata){
		std::vector<char> *v = reinterpret_cast<std::vector<char>*>(userdata);
		v->reserve(v->size() + size * nmemb + 1);
		for(size_t i = 0; i < size * nmemb; ++i){ v->push_back(buffer[i]); }
		return size * nmemb;
	}

public:
	RemoteQueryEngine()
		: m_curl(nullptr, curl_easy_cleanup)
		, m_endpoint()
	{ }

	explicit RemoteQueryEngine(std::string endpoint)
		: m_curl(curl_easy_init(), &curl_easy_cleanup)
		, m_endpoint(std::move(endpoint))
	{
		if(!m_curl){ throw std::runtime_error("failed to initialize libcurl"); }
		curl_easy_setopt(m_curl.get(), CURLOPT_URL, m_endpoint.c_str());
		curl_easy_setopt(m_curl.get(), CURLOPT_WRITEFUNCTION, callback);
	}

	std::string operator()(const std::string& body){
		std::vector<char> received_raw;
		curl_easy_setopt(m_curl.get(), CURLOPT_POSTFIELDS, body.c_str());
		curl_easy_setopt(m_curl.get(), CURLOPT_WRITEDATA, &received_raw);
		curl_easy_perform(m_curl.get());
		received_raw.push_back('\0');
		return std::string(received_raw.data());
	}

};


//----------------------------------------------------------------------------
// Structures
//----------------------------------------------------------------------------
enum class PlayerRole {
	ATTACKER = 0,
	DEFENDER = 1
};

std::ostream& operator<<(std::ostream& os, PlayerRole x){
	switch(x){
	case PlayerRole::ATTACKER: return os << "Attacker";
	case PlayerRole::DEFENDER: return os << "Defender";
	default: return os << "(unknown)";
	}
}

enum class GameStage {
	NOT_STARTED = 0,
	RUNNING     = 1,
	COMPLETED   = 2
};

std::ostream& operator<<(std::ostream& os, GameStage x){
	switch(x){
	case GameStage::NOT_STARTED: return os << "NotStarted";
	case GameStage::RUNNING:     return os << "Running";
	case GameStage::COMPLETED:   return os << "Completed";
	default: return os << "(unknown)";
	}
}


struct RoomInfo {
	long attacker_key;
	long defender_key;

	RoomInfo()
		: attacker_key(0)
		, defender_key(0)
	{ }
};


struct ShipParams {
	long x0, x1, x2, x3;

	ShipParams() : x0(0), x1(0), x2(0), x3(0) { }

	Element encode() const {
		std::vector<Element> root;
		root.emplace_back(x0);
		root.emplace_back(x1);
		root.emplace_back(x2);
		root.emplace_back(x3);
		return Element(std::move(root));
	}

	static ShipParams decode(const Element& e){
		if(e.is_nil()){ return ShipParams(); }
		const auto& e_list = e.as_list();
		ShipParams params;
		params.x0 = e_list[0].as_number();
		params.x1 = e_list[1].as_number();
		params.x2 = e_list[2].as_number();
		params.x3 = e_list[3].as_number();
		return params;
	}

	void dump(std::ostream& os, size_t depth = 0) const {
		const std::string prefix(depth * 2, ' ');
		os << prefix << "x0: " << x0 << std::endl;
		os << prefix << "x1: " << x1 << std::endl;
		os << prefix << "x2: " << x2 << std::endl;
		os << prefix << "x3: " << x3 << std::endl;
	}
};


class CommandListBuilder {

private:
	std::vector<Element> m_commands;

public:
	CommandListBuilder()
		: m_commands()
	{ }

	Element build() const {
		return Element(m_commands);
	}

	void accel(long ship_id, const Vec& v){
		std::vector<Element> root;
		root.emplace_back(0);
		root.emplace_back(ship_id);
		root.emplace_back(v);
		m_commands.push_back(std::move(root));
	}

	void detonate(long ship_id){
		std::vector<Element> root;
		root.emplace_back(1);
		root.emplace_back(ship_id);
		m_commands.push_back(std::move(root));
	}

	void shoot(long ship_id, const Vec& target){
		std::vector<Element> root;
		root.emplace_back(2);
		root.emplace_back(ship_id);
		root.emplace_back(target);
		root.emplace_back(0);
		m_commands.push_back(std::move(root));
	}

};


struct StaticGameInfo {
	long       time_limit;
	PlayerRole self_role;

	StaticGameInfo()
		: time_limit(0)
		, self_role(PlayerRole::ATTACKER)
	{ }

	static StaticGameInfo decode(const Element& e){
		if(e.is_nil()){ return StaticGameInfo(); }
		const auto& e_list = e.as_list();
		StaticGameInfo info;
		info.time_limit = e_list[0].as_number();
		info.self_role  = static_cast<PlayerRole>(e_list[1].as_number());
		return info;
	}

	void dump(std::ostream& os, size_t depth = 0) const {
		const std::string prefix(depth * 2, ' ');
		os << prefix << "time_limit: " << time_limit << std::endl;
		os << prefix << "self_role: " << self_role << std::endl;
	}
};

struct ShipState {
	PlayerRole role;
	long       id;
	Vec        pos;
	Vec        vel;
	ShipParams params;

	ShipState()
		: role(PlayerRole::ATTACKER)
		, id(0)
		, pos()
		, vel()
		, params()
	{ }

	static ShipState decode(const Element& e){
		if(e.is_nil()){ return ShipState(); }
		const auto& e_list = e.as_list();
		ShipState state;
		state.role   = static_cast<PlayerRole>(e_list[0].as_number());
		state.id     = e_list[1].as_number();
		state.pos    = e_list[2].as_vector();
		state.vel    = e_list[3].as_vector();
		state.params = ShipParams::decode(e_list[4]);
		return state;
	}

	void dump(std::ostream& os, size_t depth = 0) const {
		const std::string prefix(depth * 2, ' ');
		os << prefix << "role: " << role << std::endl;
		os << prefix << "id: " << id << std::endl;
		os << prefix << "pos: (" << pos.x << ", " << pos.y << ")" << std::endl;
		os << prefix << "vel: (" << vel.x << ", " << vel.y << ")" << std::endl;
		os << prefix << "params:" << std::endl;
		params.dump(os, depth + 1);
	}
};

struct ShipAndCommands {
	ShipState ship;
	// TODO applied_commands;

	ShipAndCommands()
		: ship()
	{ }

	static ShipAndCommands decode(const Element& e){
		if(e.is_nil()){ return ShipAndCommands(); }
		const auto& e_list = e.as_list();
		ShipAndCommands sac;
		sac.ship = ShipState::decode(e_list[0]);
		return sac;
	}

	void dump(std::ostream& os, size_t depth = 0) const {
		const std::string prefix(depth * 2, ' ');
		os << prefix << "ship:" << std::endl;
		ship.dump(os, depth + 1);
	}
};

struct GameState {
	long                         elapsed;
	std::vector<ShipAndCommands> ships;

	GameState()
		: elapsed(0)
		, ships()
	{ }

	static GameState decode(const Element& e){
		if(e.is_nil()){ return GameState(); }
		const auto& e_list = e.as_list();
		GameState state;
		state.elapsed = e_list[0].as_number();
		const auto& raw_sac_list = e_list[2].as_list();
		for(const auto& raw_sac : raw_sac_list){
			state.ships.push_back(ShipAndCommands::decode(raw_sac));
		}
		return state;
	}

	void dump(std::ostream& os, size_t depth = 0) const {
		const std::string prefix(depth * 2, ' ');
		os << prefix << "elapsed: " << elapsed << std::endl;
		for(size_t i = 0; i < ships.size(); ++i){
			os << prefix << "ships[" << i << "]:" << std::endl;
			ships[i].dump(os, depth + 1);
		}
	}
};

struct GameResponse {
	GameStage      stage;
	StaticGameInfo static_info;
	GameState      state;

	GameResponse()
		: stage(GameStage::NOT_STARTED)
		, static_info()
		, state()
	{ }

	static GameResponse decode(const Element& e){
		if(e.is_nil()){ return GameResponse(); }
		const auto& e_list = e.as_list();
		GameResponse res;
		res.stage       = static_cast<GameStage>(e_list[1].as_number());
		res.static_info = StaticGameInfo::decode(e_list[2]);
		res.state       = GameState::decode(e_list[3]);
		return res;
	}

	void dump(std::ostream& os, size_t depth = 0) const {
		const std::string prefix(depth * 2, ' ');
		os << prefix << "stage: " << stage << std::endl;
		os << prefix << "static_info:" << std::endl;
		static_info.dump(os, depth + 1);
		os << prefix << "state:" << std::endl;
		state.dump(os, depth + 1);
	}
};


//----------------------------------------------------------------------------
// Context
//----------------------------------------------------------------------------
class GalaxyContext {

private:
	const long m_player_key;

	RemoteQueryEngine m_remote;

	Element construct_create_room_query() const {
		std::vector<Element> root;
		root.emplace_back(1);
		root.emplace_back(0);
		return Element(std::move(root));
	}

	Element construct_join_query() const {
		std::vector<Element> root;
		root.emplace_back(2);
		root.emplace_back(m_player_key);
		root.emplace_back();
		return Element(std::move(root));
	}

	Element construct_start_query(const ShipParams& params) const {
		std::vector<Element> root;
		root.emplace_back(3);
		root.emplace_back(m_player_key);
		root.emplace_back(params.encode());
		return Element(std::move(root));
	}

	Element construct_command_query(Element e) const {
		std::vector<Element> root;
		root.emplace_back(4);
		root.emplace_back(m_player_key);
		root.emplace_back(std::move(e));
		return Element(std::move(root));
	}

public:
	GalaxyContext()
		: m_player_key(0)
		, m_remote()
	{ }

	GalaxyContext(std::string endpoint, long player_key)
		: m_player_key(player_key)
		, m_remote(std::move(endpoint))
	{ }

	RoomInfo create_room(){
		const auto q = construct_create_room_query();
#ifdef GALAXY_VERBOSE
		std::cerr << "<< " << serialize(q) << std::endl;
#endif
		const auto res = demodulate(m_remote(modulate(q)));
#ifdef GALAXY_VERBOSE
		std::cerr << ">> " << serialize(res) << std::endl;
#endif
		RoomInfo ri;
		for(const auto& e : res.as_list()[1].as_list()){
			const auto vs = e.as_list();
			if(vs[0].as_number() == 0){
				ri.attacker_key = vs[1].as_number();
			}else{
				ri.defender_key = vs[1].as_number();
			}
		}
		return ri;
	}

	GameResponse join(){
		const auto q = construct_join_query();
#ifdef GALAXY_VERBOSE
		std::cerr << "<< " << serialize(q) << std::endl;
#endif
		const auto res = demodulate(m_remote(modulate(q)));
#ifdef GALAXY_VERBOSE
		std::cerr << ">> " << serialize(res) << std::endl;
#endif
		return GameResponse::decode(res);
	}

	GameResponse start(const ShipParams& params){
		const auto q = construct_start_query(params);
#ifdef GALAXY_VERBOSE
		std::cerr << "<< "  << serialize(q) << std::endl;
#endif
		const auto res = demodulate(m_remote(modulate(q)));
#ifdef GALAXY_VERBOSE
		std::cerr << ">> " << serialize(res) << std::endl;
#endif
		return GameResponse::decode(res);
	}

	GameResponse command(const CommandListBuilder& cl){
		const auto q = construct_command_query(cl.build());
#ifdef GALAXY_VERBOSE
		std::cerr << "<< "  << serialize(q) << std::endl;
#endif
		const auto res = demodulate(m_remote(modulate(q)));
#ifdef GALAXY_VERBOSE
		std::cerr << ">> " << serialize(res) << std::endl;
#endif
		return GameResponse::decode(res);
	}

};

}

#endif
