#include <iostream>
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
	NUMBER    = 0,
	REFERENCE = 1,
	APPLY     = 2,
};

struct Node {
	Kind kind;
	long number;
	std::string key;
	std::shared_ptr<Node> fn;
	std::shared_ptr<Node> arg;
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
	if(std::isdigit(token[0])){
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
	virtual long value() const { throw std::runtime_error("value() is not implemented"); }
	virtual bool is_nil() const { return false; }
	virtual bool is_zero() const { return false; }
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
			return Impl::call(m_arg0, std::move(arg1));
		}
	};
public:
	virtual ObjectPtr call(NodePtr arg){
		return std::make_shared<T1>(std::move(arg));
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
			return Impl::call(m_arg0, m_arg1, std::move(arg2));
		}
	};
	class T1 : public Object {
	private:
		NodePtr m_arg0;
	public:
		T1(NodePtr arg0) : m_arg0(std::move(arg0)) { }
		virtual ObjectPtr call(NodePtr arg1){
			return std::make_shared<T2>(m_arg0, std::move(arg1));
		}
	};
public:
	virtual ObjectPtr call(NodePtr arg){
		return std::make_shared<T1>(std::move(arg));
	}
};

ObjectPtr evaluate(NodePtr node);

// #1, 2, 3 - Numbers
struct Number : public Object {
private:
	long m_value;
public:
	explicit Number(long x) : m_value(x) { }
	virtual long value() const override { return m_value; }
	virtual bool is_zero() const override { return m_value == 0; }
	virtual ObjectPtr call(NodePtr) override { throw std::runtime_error("not callable"); }
	virtual void dump(std::ostream& os) const override { os << m_value; }
};

// #5 - Successor
struct Inc : public Object {
	virtual ObjectPtr call(NodePtr arg) override {
		return std::make_shared<Number>(evaluate(arg)->value() + 1);
	}
};

// #6 - Predecessor
struct Dec : public Object {
	virtual ObjectPtr call(NodePtr arg) override {
		return std::make_shared<Number>(evaluate(arg)->value() - 1);
	}
};

// #7 - Sum
struct SumImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1){
		return std::make_shared<Number>(evaluate(x0)->value() + evaluate(x1)->value());
	}
};
using Sum = ObjectHelper2<SumImpl>;

// #9 - Product
struct ProdImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1){
		return std::make_shared<Number>(evaluate(x0)->value() * evaluate(x1)->value());
	}
};
using Prod = ObjectHelper2<ProdImpl>;

// #10 - Integer Division
struct DivImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1){
		return std::make_shared<Number>(evaluate(x0)->value() / evaluate(x1)->value());
	}
};
using Div = ObjectHelper2<DivImpl>;

// #21 - True (K Combinator)
struct TrueImpl {
	static ObjectPtr call(NodePtr x0, NodePtr){
		return evaluate(std::move(x0));
	}
};
using True = ObjectHelper2<TrueImpl>;

// #22 - False
struct FalseImpl {
	static ObjectPtr call(NodePtr, NodePtr x1){
		return evaluate(std::move(x1));
	}
};
using False = ObjectHelper2<FalseImpl>;

// #11 - Equality and Booleans
struct EqImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1){
		const long x = evaluate(std::move(x0))->value();
		const long y = evaluate(std::move(x1))->value();
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
		const long x = evaluate(std::move(x0))->value();
		const long y = evaluate(std::move(x1))->value();
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
		return std::make_shared<Number>(-evaluate(std::move(x0))->value());
	}
};

// #18 - S Combinator
struct SImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1, NodePtr x2){
		auto tmp = std::make_shared<Node>();
		tmp->kind = Kind::APPLY;
		tmp->fn   = std::move(x1);
		tmp->arg  = x2;
		return evaluate(x0)->call(std::move(x2))->call(std::move(tmp));
	}
};
using S = ObjectHelper3<SImpl>;

// #19 - C Combinator
struct CImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1, NodePtr x2){
		return evaluate(std::move(x0))->call(std::move(x2))->call(std::move(x1));
	}
};
using C = ObjectHelper3<CImpl>;

// #20 - B Combinator
struct BImpl {
	static ObjectPtr call(NodePtr x0, NodePtr x1, NodePtr x2){
		auto tmp = std::make_shared<Node>();
		tmp->kind = Kind::APPLY;
		tmp->fn   = std::move(x1);
		tmp->arg  = std::move(x2);
		return evaluate(std::move(x0))->call(std::move(tmp));
	}
};
using B = ObjectHelper3<BImpl>;

// #24 - I Combinator
struct I : public Object {
	virtual ObjectPtr call(NodePtr x0) override {
		return evaluate(std::move(x0));
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
			return evaluate(std::move(arg2))->call(m_arg0)->call(m_arg1);
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
		return evaluate(x0)->call(std::move(tmp));
	}
};

// #27 - Cdr (Tail)
struct Cdr : public Object {
	virtual ObjectPtr call(NodePtr x0) override {
		auto tmp = std::make_shared<Node>();
		tmp->kind = Kind::REFERENCE;
		tmp->key  = "f";
		return evaluate(x0)->call(std::move(tmp));
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
		if(evaluate(std::move(arg))->is_nil()){
			return std::make_shared<True>();
		}else{
			return std::make_shared<False>();
		}
	}
};

std::shared_ptr<Object> evaluate(NodePtr node){
	if(node->kind == Kind::NUMBER){
		return std::make_shared<Number>(node->number);
	}else if(node->kind == Kind::REFERENCE){
		const auto& k = node->key;
		if(node->key == "inc")  { return std::make_shared<Inc>();    }
		if(node->key == "dec")  { return std::make_shared<Dec>();    }
		if(node->key == "add")  { return std::make_shared<Sum>();    }
		if(node->key == "mul")  { return std::make_shared<Prod>();   }
		if(node->key == "div")  { return std::make_shared<Div>();    }
		if(node->key == "eq")   { return std::make_shared<Eq>();     }
		if(node->key == "lt")   { return std::make_shared<Lt>();     }
		if(node->key == "neg")  { return std::make_shared<Negate>(); }
		if(node->key == "s")    { return std::make_shared<S>();      }
		if(node->key == "c")    { return std::make_shared<C>();      }
		if(node->key == "b")    { return std::make_shared<B>();      }
		if(node->key == "t")    { return std::make_shared<True>();   }
		if(node->key == "f")    { return std::make_shared<False>();  }
		// TODO pwr2
		if(node->key == "i")    { return std::make_shared<I>();      }
		if(node->key == "cons") { return std::make_shared<Cons>();   }
		if(node->key == "car")  { return std::make_shared<Car>();    }
		if(node->key == "cdr")  { return std::make_shared<Cdr>();    }
		if(node->key == "nil")  { return std::make_shared<Nil>();    }
		if(node->key == "isnil"){ return std::make_shared<IsNil>();  }
		return evaluate(g_slots[k]);
	}else if(node->kind == Kind::APPLY){
		return evaluate(node->fn)->call(node->arg);
	}
	return nullptr;
}


int main(int argc, char *argv[]){
	std::string line;
	while(std::getline(std::cin, line)){
		std::istringstream iss(line);
		std::string key, eq;
		iss >> key >> eq;
		g_slots[key] = std::make_shared<Node>(parse(iss));
		// evaluate(std::make_shared<Node>(parse(iss)))->dump(std::cout);
		// std::cout << std::endl;
	}
	evaluate(g_slots[argv[1]])->dump(std::cout);
	std::cout << std::endl;
	return 0;
}

