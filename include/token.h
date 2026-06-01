#ifndef TOKEN_H
#define TOKEN_H

#include "primitives.h"

class lexicalAnalysis {
public:
    struct Token {
        string type;
        string value;
    };

private:
    static constexpr const char* KEYWORD = "KEYWORD";
    static constexpr const char* OPERATOR = "OPERATOR";
    static constexpr const char* SYMBOL = "SYMBOL";
    static constexpr const char* GATE = "GATE";
    static constexpr const char* LITERAL = "LITERAL";
    static constexpr const char* COMMENT = "COMMENT";
    static constexpr const char* MODULE = "MODULE";
    static constexpr const char* IDENTIFIER = "IDENTIFIER";

    const unordered_set<string> keywords = {
        "input", "output", "wire", "logic", "reg"
    };

    const unordered_set<string> gates = {
        "and", "or", "not", "nand", "nor", "xor", "xnor", "buf"
    };

    const unordered_set<string> symbols = {
        "(", ")", "{", "}", "[", "]", ",", ";", ":"
    };

    const unordered_set<string> operators = {
        "+", "-", "*", "/", "=", "==", "!=", "<", ">", 
        "<=", ">=", "&", "|", "^", "~", "+=", "-=", 
        "*=", "/=", "#"
    };

    const unordered_set<string> two_char_operators = {
        "==", "+=", "-=", "*=", "/=", "!=", ">=", "<="
    };

    vector<Token> tokens;

    bool isWhitespace(char c) const {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    bool isSingleCharToken(char c) const {
        return c == ',' || c == ';' || c == '(' || c == ')' || 
               c == '=' || c == '{' || c == '}' || c == '[' || 
               c == ']' || c == ':';
    }

    bool isPotentialTwoCharOperator(char c1, char c2) const {
        string potential_op = string(1, c1) + string(1, c2);
        return two_char_operators.find(potential_op) != two_char_operators.end();
    }

    vector<string> splitIntoTokens(const string& file) const {
        vector<string> token_strings;
        string current_token;

        for (size_t i = 0; i < file.size(); i++) {
            char c = file[i];

            if (i + 1 < file.size() && isPotentialTwoCharOperator(c, file[i+1])) {
                if (!current_token.empty()) {
                    token_strings.push_back(current_token);
                    current_token.clear();
                }
                token_strings.push_back(string(1, c) + string(1, file[i+1]));
                i++;
                continue;
            }

            if (isWhitespace(c) || isSingleCharToken(c)) {
                if (!current_token.empty()) {
                    token_strings.push_back(current_token);
                    current_token.clear();
                }
                if (isSingleCharToken(c)) {
                    token_strings.push_back(string(1, c));
                }
            } else {
                current_token += c;
            }
        }

        if (!current_token.empty()) {
            token_strings.push_back(current_token);
        }

        return token_strings;
    }

    vector<Token> classifyTokens(const vector<string>& token_strings) const {
        vector<Token> classified_tokens;

        for (const string& token_str : token_strings) {
            Token token;
            token.value = token_str;

            if (keywords.count(token_str)) {
                token.type = KEYWORD;
            } else if (operators.count(token_str)) {
                token.type = OPERATOR;
            } else if (symbols.count(token_str)) {
                token.type = SYMBOL;
            } else if (gates.count(token_str)) {
                token.type = GATE;
            } else if (token_str == "module" || token_str == "endmodule") {
                token.type = MODULE;
            } else if (isdigit(token_str[0]) || (token_str.size() > 1 && token_str[0] == '\'')) {
                token.type = LITERAL;
            } else if (token_str == "//" || token_str == "/*" || token_str == "*/") {
                token.type = COMMENT;
            } else {
                token.type = IDENTIFIER;
            }

            classified_tokens.push_back(token);
        }

        return classified_tokens;
    }

public:

    lexicalAnalysis(const string& file_content) {
        vector<string> token_strings = splitIntoTokens(file_content);
        tokens = classifyTokens(token_strings);
    }

    const vector<Token>& getTokens() const {
        return tokens;
    }

    static vector<Token> tokenize(const string& file) {
        lexicalAnalysis tokenizer(file);
        return tokenizer.getTokens();
    }
};



// GPT code for print token
void print_tokens(const std::vector<lexicalAnalysis::Token>& tokens) 
{
    cout << std::left 
         << std::setw(15) << "Token Type" 
         << std::setw(25) << "Value" 
         << "Position\n";
    cout << std::setfill('-') 
         << std::setw(15) << "" 
         << std::setw(25) << "" 
         << std::setw(10) << "" 
         << std::setfill(' ') << "\n";


    for (size_t i = 0; i < tokens.size(); ++i) {
        const auto& token = tokens[i];
        std::cout << std::left 
                  << std::setw(15) << token.type 
                  << std::setw(25) << token.value 
                  << "[" << i << "]\n";
        
        if (token.type == "COMMENT") {
            std::cout << std::setfill('-') << std::setw(50) << "" 
                      << std::setfill(' ') << "\n";
        }
    }
    
    cout << "\nTotal tokens: " << tokens.size() << "\n";
}

#endif //TOKEN_H
