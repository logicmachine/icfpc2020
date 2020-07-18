#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdexcept>
#include <cstdint>

class Object;
using ObjectPtr = std::shared_ptr<Object>;

enum class Kind {
	OBJECT    = 0,
	NUMBER    = 1,
	REFERENCE = 2,
	APPLY     = 3,
};

struct Node {
	Kind kind;
	long number;
	std::string key;
	std::shared_ptr<Node> fn;
	std::shared_ptr<Node> arg;
	ObjectPtr cache;
};
using NodePtr = std::shared_ptr<Node>;

std::ostream& operator<<(std::ostream& os, Node& node){
	if(node.kind == Kind::NUMBER){
		os << node.number;
	}else if(node.kind == Kind::REFERENCE){
		os << node.key;
	}else if(node.kind == Kind::APPLY){
		os << *(node.fn) << "(" << *(node.arg) << ")";
	}
	return os;
}

Node parse(std::istream& is){
	std::string token;
	is >> token;
	if(token == ""){ throw std::runtime_error("empty token"); }
	Node node;
	if(token[0] == '-' || std::isdigit(token[0])){
		node.kind = Kind::NUMBER;
		node.number = std::stol(token);
	}else if(token == "ap"){
		node.kind = Kind::APPLY;
		node.fn   = std::make_shared<Node>(parse(is));
		node.arg  = std::make_shared<Node>(parse(is));
	}else{
		node.kind = Kind::REFERENCE;
		node.key  = token;
	}
	return node;
}

static std::map<std::string, std::shared_ptr<Node>> g_slots;

//----------------------------------------------------------------------------
// Function declarations
//----------------------------------------------------------------------------
struct Object {
	virtual bool is_nil()       const { return false; }
	virtual bool is_number()    const { return false; }
	virtual bool is_modulated() const { return false; }

	virtual long number() const { throw std::runtime_error("object is not a number"); }
	virtual std::string modulated() const { throw std::runtime_error("object is not a modulated"); }

	virtual ObjectPtr call(NodePtr arg) = 0;

	virtual void dump(std::ostream& os) const { throw std::runtime_error("dump() is not implemented"); }
};

template <typename Impl>
struct ObjectHelper2 : public Object{
private:
	class T1 : public Object {
	private:
		NodePtr m_arg0;
	public:
		T1(NodePtr arg0) : m_arg0(std::move(arg0)) { }
		virtual ObjectPtr call(NodePtr arg1){
			return Impl::call(m_arg0, arg1);
		}
	};
public:
	virtual ObjectPtr call(NodePtr arg){
		return std::make_shared<T1>(arg);
	}
};

template <typename Impl>
struct ObjectHelper3 : public Object{
private:
	class T2 : public Object {
	private:
		NodePtr m_arg0, m_arg1;
	public:
		T2(NodePtr arg0, NodePtr arg1) : m_arg0(std::move(arg0)), m_arg1(std::move(arg1)) { }
		virtual ObjectPtr call(NodePtr arg2){
			return Impl::call(m_arg0, m_arg1, arg2);
		}
	};
	class T1 : public Object {
	private:
		NodePtr m_arg0;
	public:
		T1(NodePtr arg0) : m_arg0(std::move(arg0)) { }
		virtual ObjectPtr call(NodePtr arg1){
			return std::make_shared<T2>(m_arg0, arg1);
		}
	};
public:
	virtual ObjectPtr call(NodePtr arg){
		return std::make_shared<T1>(arg);
	}
};

ObjectPtr evaluate(NodePtr node);

NodePtr as_node(ObjectPtr obj){
	auto node = std::make_shared<Node>();
	node->kind  = Kind::OBJECT;
	node->cache = obj;
	return node;
}

ObjectPtr apply(ObjectPtr fn, ObjectPtr arg){
	auto node = std::make_shared<Node>();
	node->kind = Kind::APPLY;
	node->fn   = as_node(fn);
	node->arg  = as_node(arg);
	return evaluate(node);
}

// #1, 2, 3 - Numbers
struct Number : public Object {
private:
	long m_value;
public:
	explicit Number(long x) : m_value(x) { }
	virtual bool is_number() const override { return true; }
	virtual long number()    const override { return m_value; }
	virtual ObjectPtr call(NodePtr) override { throw std::runtime_error("number is not a callable"); }
	virtual void dump(std::ostream& os) const override { os << m_value; }
};

// #5 - Successor
struct Inc : public Object {
	virtual ObjectPtr call(NodePtr arg) override {
		return std::make_shared<Number>(evaluate(arg)->number() + 1);
	}
};

// #6 - Predecessor
struct Dec : public Object {
	virtual ObjectPtr call(NodePtr arg) override {
		return std::make_shared<Number>(evaluate(arg)->number() - 1);
	}
};

// #7 - Sum
struct SumImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1){
		return std::make_shared<Number>(evaluate(x0)->number() + evaluate(x1)->number());
	}
};
using Sum = ObjectHelper2<SumImpl>;

// #9 - Product
struct ProdImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1){
		return std::make_shared<Number>(evaluate(x0)->number() * evaluate(x1)->number());
	}
};
using Prod = ObjectHelper2<ProdImpl>;

// #10 - Integer Division
struct DivImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1){
		return std::make_shared<Number>(evaluate(x0)->number() / evaluate(x1)->number());
	}
};
using Div = ObjectHelper2<DivImpl>;

// #21 - True (K Combinator)
struct TrueImpl {
	static ObjectPtr call(NodePtr x0, NodePtr){
		return evaluate(x0);
	}
};
using True = ObjectHelper2<TrueImpl>;

// #22 - False
struct FalseImpl {
	static ObjectPtr call(NodePtr, NodePtr x1){
		return evaluate(x1);
	}
};
using False = ObjectHelper2<FalseImpl>;

// #11 - Equality and Booleans
struct EqImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1){
		const long x = evaluate(x0)->number();
		const long y = evaluate(x1)->number();
		if(x == y){
			return std::make_shared<True>();
		}else{
			return std::make_shared<False>();
		}
	}
};
using Eq = ObjectHelper2<EqImpl>;

// #12 - Strict Less Than
struct LtImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1){
		const long x = evaluate(x0)->number();
		const long y = evaluate(x1)->number();
		if(x < y){
			return std::make_shared<True>();
		}else{
			return std::make_shared<False>();
		}
	}
};
using Lt = ObjectHelper2<LtImpl>;

// #16 - Negate
struct Negate : public Object {
	virtual ObjectPtr call(NodePtr x0) override {
		return std::make_shared<Number>(-evaluate(x0)->number());
	}
};

// #18 - S Combinator
struct SImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1, NodePtr x2){
		auto tmp = std::make_shared<Node>();
		tmp->kind = Kind::APPLY;
		tmp->fn   = x1;
		tmp->arg  = x2;
		return evaluate(x0)->call(x2)->call(tmp);
	}
};
using S = ObjectHelper3<SImpl>;

// #19 - C Combinator
struct CImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1, NodePtr x2){
		return evaluate(x0)->call(x2)->call(x1);
	}
};
using C = ObjectHelper3<CImpl>;

// #20 - B Combinator
struct BImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1, NodePtr x2){
		auto tmp = std::make_shared<Node>();
		tmp->kind = Kind::APPLY;
		tmp->fn   = x1;
		tmp->arg  = x2;
		return evaluate(x0)->call(tmp);
	}
};
using B = ObjectHelper3<BImpl>;

// #24 - I Combinator
struct I : public Object {
	virtual ObjectPtr call(NodePtr x0) override {
		return evaluate(x0);
	}
};

// #25 - Cons
class Cons : public Object {
private:
	class Cons2 : public Object {
	private:
		NodePtr m_arg0, m_arg1;
	public:
		Cons2(NodePtr arg0, NodePtr arg1) : m_arg0(arg0), m_arg1(arg1) { }
		virtual ObjectPtr call(NodePtr arg2) override {
			return evaluate(arg2)->call(m_arg0)->call(m_arg1);
		}
		virtual void dump(std::ostream& os) const override {
			os << "(";
			evaluate(m_arg0)->dump(os);
			os << ", ";
			evaluate(m_arg1)->dump(os);
			os << ")";
		}
	};
	class Cons1 : public Object {
	private:
		NodePtr m_arg0;
	public:
		explicit Cons1(NodePtr arg0) : m_arg0(arg0) { }
		virtual ObjectPtr call(NodePtr arg1) override {
			return std::make_shared<Cons2>(m_arg0, arg1);
		}
	};
public:
	virtual ObjectPtr call(NodePtr arg){
		return std::make_shared<Cons1>(arg);
	}
};

// #26 - Car (First)
struct Car : public Object {
	virtual ObjectPtr call(NodePtr x0) override {
		auto tmp = std::make_shared<Node>();
		tmp->kind = Kind::REFERENCE;
		tmp->key  = "t";
		return evaluate(x0)->call(tmp);
	}
};

// #27 - Cdr (Tail)
struct Cdr : public Object {
	virtual ObjectPtr call(NodePtr x0) override {
		auto tmp = std::make_shared<Node>();
		tmp->kind = Kind::REFERENCE;
		tmp->key  = "f";
		return evaluate(x0)->call(tmp);
	}
};

// #28 - Nil
class Nil : public Object {
public:
	virtual bool is_nil() const override { return true; }
	virtual ObjectPtr call(NodePtr arg) override {
		return std::make_unique<True>();
	}
	virtual void dump(std::ostream& os) const override { os << "nil"; }
};

// #29 - Is Nil
class IsNil : public Object {
public:
	virtual ObjectPtr call(NodePtr arg) override {
		if(evaluate(arg)->is_nil()){
			return std::make_shared<True>();
		}else{
			return std::make_shared<False>();
		}
	}
};

// #37 - Is Zero
class IsZero : public Object {
public:
	virtual ObjectPtr call(NodePtr arg) override {
		auto t = evaluate(arg);
		if(t->is_number() && t->number() == 0){
			return std::make_shared<True>();
		}else{
			return std::make_shared<False>();
		}
	}
};

// #13 - Modulate
struct Modulated : public Object {
private:
	std::string m_signal;
public:
	explicit Modulated(std::string signal) : m_signal(std::move(signal)) { }
	virtual bool is_modulated() const override { return true; }
	virtual std::string modulated() const override { return m_signal; }
	virtual ObjectPtr call(NodePtr) override { throw std::runtime_error("modulated is not a callable"); }
	virtual void dump(std::ostream& os) const override { os << "[" << m_signal << "]"; }
};

struct Modulate : public Object {
private:
	static void impl(std::ostream& os, ObjectPtr cur){
		if(cur->is_number()){
			const long x = cur->number();
			if(x == 0){
				os << "010";
			}else{
				const long y = std::abs(x);
				os << (x < 0 ? "01" : "10");
				const int bits = (sizeof(long) * 8 - __builtin_clzl(y) + 3) & ~3;
				for(int i = 0; i < bits; i += 4){ os << "1"; }
				os << "0";
				for(int i = bits - 1; i >= 0; --i){ os << ((y >> i) & 1); }
			}
		}else if(cur->is_nil()){
			os << "00";
		}else{
			os << "11";
			impl(os, apply(std::make_shared<Car>(), cur));
			impl(os, apply(std::make_shared<Cdr>(), cur));
		}
	}
public:
	virtual ObjectPtr call(NodePtr arg) override {
		std::ostringstream oss;
		impl(oss, evaluate(arg));
		return std::make_shared<Modulated>(oss.str());
	}
};

// #14 - Demodulate
struct Demodulate : public Object {
private:
	static ObjectPtr impl(std::istream& is){
		const char m0 = is.get();
		const char m1 = is.get();
		if(m0 != m1){
			const long sign = (m0 == '0' ? 1 : -1);
			int bits = 0;
			while(is.get() == '1'){ bits += 4; }
			long value = 0;
			for(int i = 0; i < bits; ++i){
				value = (value << 1) | (is.get() - '0');
			}
			return std::make_shared<Number>(sign * value);
		}else if(m0 == '0'){
			return std::make_shared<Nil>();
		}else if(m0 == '1'){
			auto first = impl(is);
			auto tail  = impl(is);
			return apply(apply(std::make_shared<Cons>(), first), tail);
		}
		return nullptr;
	}
public:
	virtual ObjectPtr call(NodePtr arg) override {
		auto value = evaluate(arg);
		if(!value->is_modulated()){ throw std::runtime_error("value is not modulated"); }
		std::istringstream iss(value->modulated());
		return impl(iss);
	}
};

// #15 - Send
struct Send : public Object {
	virtual ObjectPtr call(NodePtr arg) override {
		const auto signal = apply(std::make_shared<Modulate>(), evaluate(arg));
		std::cout << "Send: " << signal->modulated() << std::endl;
		std::cout << "? " << std::flush;
		std::string received;
		std::cin >> received;
		return apply(std::make_shared<Demodulate>(), std::make_shared<Modulated>(received));
	}
};

// #38 - Interact
struct InteractImpl {
	static ObjectPtr call(NodePtr protocol, NodePtr state, NodePtr vector){
		auto t = evaluate(protocol)->call(state)->call(vector);
		auto flag = apply(std::make_shared<Car>(), t);
		if(flag->number() == 0){
			// TODO multipledraw
			auto ret = apply(std::make_shared<Cdr>(), t);
			g_slots[":state"] = as_node(apply(std::make_shared<Car>(), ret));
			return ret;
		}else{
			auto ret   = apply(std::make_shared<Cdr>(), t);
			auto state = apply(std::make_shared<Car>(), ret);
			auto data  = apply(std::make_shared<Cdr>(), ret);
			auto recv  = apply(std::make_shared<Send>(), data);
recv->dump(std::cout); std::cout << std::endl;
			auto interact = std::make_shared<ObjectHelper3<InteractImpl>>();
			return interact->call(protocol)->call(as_node(state))->call(as_node(recv));
		}
	}
};
using Interact = ObjectHelper3<InteractImpl>;


std::shared_ptr<Object> evaluate(NodePtr node){
	auto factory = [&]() -> ObjectPtr {
		if(node->kind == Kind::NUMBER){
			return std::make_shared<Number>(node->number);
		}else if(node->kind == Kind::REFERENCE){
			const auto& k = node->key;
			if(node->key == "inc")     { return std::make_shared<Inc>();        }
			if(node->key == "dec")     { return std::make_shared<Dec>();        }
			if(node->key == "add")     { return std::make_shared<Sum>();        }
			if(node->key == "mul")     { return std::make_shared<Prod>();       }
			if(node->key == "div")     { return std::make_shared<Div>();        }
			if(node->key == "eq")      { return std::make_shared<Eq>();         }
			if(node->key == "lt")      { return std::make_shared<Lt>();         }
			if(node->key == "mod")     { return std::make_shared<Modulate>();   }
			if(node->key == "dem")     { return std::make_shared<Demodulate>(); }
			if(node->key == "send")    { return std::make_shared<Send>();       }
			if(node->key == "neg")     { return std::make_shared<Negate>();     }
			if(node->key == "s")       { return std::make_shared<S>();          }
			if(node->key == "c")       { return std::make_shared<C>();          }
			if(node->key == "b")       { return std::make_shared<B>();          }
			if(node->key == "t")       { return std::make_shared<True>();       }
			if(node->key == "f")       { return std::make_shared<False>();      }
			// TODO pwr2
			if(node->key == "i")       { return std::make_shared<I>();          }
			if(node->key == "cons")    { return std::make_shared<Cons>();       }
			if(node->key == "car")     { return std::make_shared<Car>();        }
			if(node->key == "cdr")     { return std::make_shared<Cdr>();        }
			if(node->key == "nil")     { return std::make_shared<Nil>();        }
			if(node->key == "isnil")   { return std::make_shared<IsNil>();      }
			if(node->key == "if0")     { return std::make_shared<IsZero>();     }
			if(node->key == "interact"){ return std::make_shared<Interact>();   }
			auto t = evaluate(g_slots[k]);
			return t;
		}else if(node->kind == Kind::APPLY){
			auto t = evaluate(node->fn)->call(node->arg);
			return t;
		}
		return nullptr;
	};
	if(node->cache){ return node->cache; }
	node->cache = factory();
	return node->cache;
}


int main(int argc, char *argv[]){
	if(argc < 2){
		std::cerr << "Usage: " << argv[0] << " setup" << std::endl;
		return 0;
	}

	std::string line;

	std::ifstream ifs(argv[1]);
	while(std::getline(ifs, line)){
		std::istringstream iss(line);
		std::string key, eq;
		iss >> key >> eq;
		g_slots[key] = std::make_shared<Node>(parse(iss));
	}

	auto state = std::make_shared<Node>();
	state->kind  = Kind::OBJECT;
	state->cache = std::make_shared<Nil>();
	g_slots[":state"] = state;

	while(true){
		std::cout << "> " << std::flush;
		if(!std::getline(std::cin, line)){ break; }
		if(line.size() == 0){ continue; }
		std::istringstream iss(line);
		std::string key, eq;
		if(line[0] == ':' && line.find('=') != std::string::npos){ iss >> key >> eq; }
		auto root = std::make_shared<Node>(parse(iss));
		evaluate(root)->dump(std::cout);
		std::cout << std::endl;
		if(line[0] == ':' && line.find('=') != std::string::npos){ g_slots[key] = root; }
	}

	return 0;
}

