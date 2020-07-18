#include <cstdio>
#include <algorithm>
#include <string>
#include <cassert>

bool is_digit(char chr) {
    return static_cast<bool>(isdigit(chr));
}

bool is_minus(char chr) {
    return chr == '-';
}

bool is_alpha(char chr) {
    return static_cast<bool>(isalpha(chr));
}

bool is_sep(char chr) {
    return static_cast<bool>(std::string("(),").find(chr) != std::string::npos);
}

// ignore ' ' and ','
void ignore_unused_char(const std::string& list_str, size_t& pos) {
    while(list_str[pos] == ' ' or list_str[pos] == ',') pos++;
}

void parse(const std::string& list_str, size_t& pos, size_t& paren_depth) {
    if(list_str[pos] == '(') {
        pos++; paren_depth++; // '('
        printf("ap ap cons ");

        size_t begin_paren_depth = paren_depth, elem_cnt = 0;
        while(!(list_str[pos] == ')' and paren_depth == begin_paren_depth)) {
            parse(list_str, pos, paren_depth);

            // more than 2 blocks in parentheses is forbidden
            assert(++elem_cnt <= 2);
            ignore_unused_char(list_str, pos);
        }
        assert(list_str[pos++] == ')'); paren_depth--; // ')'
        ignore_unused_char(list_str, pos);
    }
    // number: [-]?[0-9]*
    else if (is_minus(list_str[pos]) or is_digit(list_str[pos])) {
        std::string number = "";
        number += list_str[pos++]; // the first char
        while(is_digit(list_str[pos])) {
            // space
            if(list_str[pos] == ' ') continue;
            number += list_str[pos++];
        }
        printf("%s ", number.c_str());
        ignore_unused_char(list_str, pos);
    }
    // word: [A-Za-z]+[A-Za-z0-9]*
    else if(is_alpha(list_str[pos])) {
        std::string word = "";
        while(!is_sep(list_str[pos])) {
            // space
            if(list_str[pos] == ' ') continue;
            word += list_str[pos++];
        }
        printf("%s ", word.c_str());
        ignore_unused_char(list_str, pos);
    }
    else {
        // if reached -> bug? the given list is wrong?
        assert(false);
    }
}

int main(int argc, char** argv) {
    if(argc != 2) {
        fprintf(stderr, "Usage: %s list_str\n", argv[0]);
        return 1;
    }

    std::string list_str = argv[1];
    size_t pos = 0, paren_depth = 0;
    while(pos < list_str.size()) {
        parse(list_str, pos, paren_depth);
    }
    puts("");
    return 0;
}
