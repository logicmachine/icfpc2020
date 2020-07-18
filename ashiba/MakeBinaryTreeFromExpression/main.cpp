/*
argvに式を渡すとdot形式の二分木を標準出力に出力するプログラム


コンパイル
g++ -std=c++11 main.cpp -o make_binary_tree.out

実行例
./make_binary_tree.out "ap ap add 1 2" > result.dot

Graphvizでpng画像化
dot -T png result.dot -o result.png

*/

#include <iostream>
#include <cstdlib>
#include <vector>
#include <cassert>
#include <memory>

struct Node {
    std::string val;
    int id;
    std::shared_ptr<Node> lch, rch;

    Node (const std::string& str, const int& id_num): val(str), id(id_num), lch(nullptr), rch(nullptr) {}

    bool add_left(std::shared_ptr<Node>& ptr) {
        lch = ptr;
        return true;
    }

    bool add_right(std::shared_ptr<Node>& ptr) {
        rch = ptr;
        return true;
    }
};

struct NodeStatus {
    std::string val;
    std::string label;

    NodeStatus(const std::string& str, const int& id_num): val(std::to_string(id_num)), label(str) {}
};

void dump(const std::shared_ptr<Node>& v, const std::vector<NodeStatus>& node_status_list, const std::shared_ptr<Node>& par = nullptr) {
    if (v == nullptr) {
        return ;
    }

    if (par == nullptr) {
        std::cout << "digraph G{" << std::endl;
        std::cout << "  graph [ordering=\"out\"];" << std::endl;

        std::cout << std::endl;

        for (const NodeStatus& node_status: node_status_list) {
            std::cout << node_status.val << "[label = \"" << node_status.label << "\"];" << std::endl;
        }
    }
    else {
        std::cout << "  " << par->id << " -> " << v->id << ";" << std::endl;
    }

    dump(v->lch, {}, v);
    dump(v->rch, {}, v);

    if (par == NULL)
        std::cout << "}" << std::endl;
}

bool isIncludeEquals(const std::string& str) {
    for (const char& ch: str) {
        if (ch == '=') return true;
    } 
    return false;
}

std::vector<std::string> split(const std::string& exp) {
    std::vector<std::string> ret;
    std::string buf = "";

    for (const char& ch: exp) {
        if (ch == ' ') {
            ret.emplace_back(buf);
            buf = "";
        } else {
            buf += ch;
        }
    }

    if (not buf.empty()) {
        ret.emplace_back(buf);
        buf = "";
    }

    return ret;
}

std::shared_ptr<Node> parser(int& idx, int& id_num, std::vector<NodeStatus>& node_status_list, const std::vector<std::string>& exp_list) {
    assert(idx < exp_list.size());

    node_status_list.emplace_back(NodeStatus(exp_list[idx], id_num));
    std::shared_ptr<Node> ret = std::make_shared<Node>(exp_list[idx++], id_num++);

    if (ret->val == "ap") {
        auto result = parser(idx, id_num, node_status_list, exp_list);
        ret->add_left (result);
         result = parser(idx, id_num, node_status_list, exp_list);
        ret->add_right (result);
    }

    return ret;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        std::cerr << "Arg error." << std::endl;
        return 1;
    }

    std::string exp = argv[1];
    // std::string exp = "ap ap c b ap ap s ap ap b c ap ap b ap b b ap eq 0 ap ap b ap c :1141 ap add -1";

    if (isIncludeEquals(exp)) {
        std::cerr << "exp shouldn't include equal(=)" << std::endl;
        return -1;
    }

    std::vector<std::string> exp_list = split(exp);

    int id_num = 0;
    int idx = 0;
    std::vector<NodeStatus> node_status_list;

    std::shared_ptr<Node> root = parser(idx, id_num, node_status_list, exp_list);

    assert(root != nullptr);

    dump(root, node_status_list);

    return 0;
}