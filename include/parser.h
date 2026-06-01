#ifndef PARSER_H
#define PARSER_H


#include "token.h"

enum WireType {INPUT, OUTPUT, INTERNAL, UNKNOWN };

struct gate {
    string gateType;
    string gateName;
    int To1Delay;
    int To0Delay;
    vector<string> input;
    string output;
};

struct signal {
    string name;
    WireType Type;
};

struct parseTreeNode {
    string moduleName;
    vector<gate*> gates;
    vector<signal*> wires;
};


class Parser 
{
public:

    void printParseTree(parseTreeNode* root, int level = 0) 
    {
        if (!root) return;
            cout << string(level * 2, ' ') << "Module: " << root->moduleName << std::endl;

            cout <<string(level * 2, ' ') << "Wires: " << std::endl;
            for (const auto& wire : root->wires) {
                cout << string((level + 1) * 2, ' ') << "Wire: " << wire->name << " (" 
                    << (wire->Type == INPUT ? "INPUT" : wire->Type == OUTPUT ? "OUTPUT" : "INTERNAL") 
                    << ")" << std::endl;
                }

            cout << string(level * 2, ' ') << "Gates: " << endl;
            for (const auto& gate : root->gates) {
                cout << string((level + 1) * 2, ' ') << "Gate: " << gate->gateName 
                  << " Type: " << gate->gateType 
                  << " To1Delay: " << gate->To1Delay 
                  << " To0Delay: " << gate->To0Delay 
                  << std::endl;
        
                cout << string((level + 1) * 2, ' ') << "Inputs: ";
                    for (const auto& input : gate->input) {
                    cout << input << " ";
                    }      
            cout << endl;
            cout << string((level + 1) * 2, ' ') << "Output: " << gate->output << endl;
    }
}

    Parser(const vector<lexicalAnalysis::Token>& tokens) : tokens(tokens), pos(0) {}

    parseTreeNode* parse() 
    {
        if (check_end())
            throw runtime_error("Expected token but its NULL");
        
        parseTreeNode* root = new parseTreeNode();

        parseModule(root);
        
        return root;
    }

private:
    const vector<lexicalAnalysis::Token>& tokens;
    size_t pos;

    
    bool check_end() const
    {
        return (pos >= tokens.size());
        
    }
    bool continue_if_wait_if_not(const string& expected) 
    {
        if (!check_end() && tokens[pos].value == expected) 
        {
            pos++;
            return true;
        }
        return false;
    }

    bool exist(const string& expected) const 
    {
        return (!check_end() && tokens[pos].value == expected);
    }
    
    
    void expect(const string& expected) 
    {
        if (!exist(expected)) 
        {
            throw runtime_error("Expected token: " + expected + 
                              (check_end() ? " but reached end of text" : 
                               " but got " + tokens[pos].value));
        }
        pos++;
    }
    
    void resume()
    {
        pos++;
    }


    void parseModule(parseTreeNode* root) 
    {
        if (continue_if_wait_if_not("module")) 
        {
            if (check_end())
                throw runtime_error("Unexpected end after 'module' keyword.");
        
            root->moduleName = tokens[pos].value;
            resume();

            if (continue_if_wait_if_not("(")) 
            {
                parseInputPorts(root);
                expect(")");
            }

            expect(";");

            while (!check_end() && !exist("endmodule")) 
            {
                if (tokens[pos].type == "KEYWORD") 
                {
                    parseKeyword(root);
                } 
                else if (tokens[pos].type == "GATE") 
                {
                    parseGate(root);
                } 
                else
                {
                    resume();
                }
            }
            expect("endmodule");
        }
    }

    void parseInputPorts(parseTreeNode* root) 
    {
        string previousType = "unknown";
        while (!check_end() && !exist(")")) 
        {
            signal* portNode = new signal();
            
            if ((exist("input") || previousType == "input") && !exist("output")) 
            {
                portNode->Type = INPUT;
                continue_if_wait_if_not("input");
                previousType = "input";
            } 
            else if ((exist("output") || previousType == "output") && !exist("input")) 
            {
                portNode->Type = OUTPUT;
                continue_if_wait_if_not("output");
                previousType = "output";
            } 
            else 
            {
                portNode->Type = UNKNOWN;
                previousType = "unknown";
            }
            
            if (check_end())
                throw runtime_error("Unexpected end. You must use identifier after wire declaration");

            portNode->name = tokens[pos].value;
            resume();
            root->wires.push_back(portNode);
            if (continue_if_wait_if_not(","));
        }
    }

    void parseKeyword(parseTreeNode* root) 
    {
        string keyword = tokens[pos].value;
        if (keyword == "wire") 
        {
            parseWire(root);
        } 
        else if (keyword == "input" || keyword == "output") 
        {
            parsePort(root, keyword);
        } 
        else 
        {
            resume();
        }
    }

    void parseWire(parseTreeNode* root) 
    {
        expect("wire");
        if (!check_end() && tokens[pos].type == "IDENTIFIER") 
        {
            signal* wireNode = new signal();
            wireNode->name = tokens[pos].value;
            wireNode->Type = INTERNAL;
            root->wires.push_back(wireNode);
            resume();
        } 
        else 
        {
            throw runtime_error("Invalid identifier after 'wire'");
        }

        while (continue_if_wait_if_not(",")) 
        {
            if (!check_end() && tokens[pos].type == "IDENTIFIER") 
            {
                signal* wireNode = new signal();
                wireNode->name = tokens[pos].value;
                wireNode->Type = INTERNAL;
                root->wires.push_back(wireNode);
                resume();
            }
            else 
            {
                throw runtime_error("Expected wire name after comma");
            }
        }
    }

    void parsePort(parseTreeNode* root, const string& type) 
    {
        resume();
        if (!check_end() && tokens[pos].type == "IDENTIFIER") 
        {
            signal* portNode = new signal();
            portNode->name = tokens[pos].value;
            portNode->Type = (type == "input") ? INPUT : OUTPUT;
            root->wires.push_back(portNode);
            resume();
        } 
        else 
        {
            throw runtime_error("Invalid identifier after '" + type + "'");
        }
        
        while (continue_if_wait_if_not(",")) 
        {
            if (!check_end() && tokens[pos].type == "IDENTIFIER") 
            {
                signal* portNode = new signal();
                portNode->name = tokens[pos].value;
                portNode->Type = (type == "input") ? INPUT : OUTPUT;
                root->wires.push_back(portNode);
                resume();
            }
            else 
            {
                throw runtime_error("Expected port name after comma");
            }
        }
    }

    void parseGate(parseTreeNode* root) 
    {
        gate* gateNode = new gate();
        root->gates.push_back(gateNode);
        
        gateNode->gateType = tokens[pos].value;
        resume();
        
        if (continue_if_wait_if_not("#")) 
        {
            if (continue_if_wait_if_not("(")) 
            {
                if (!check_end()) 
                {
                    gateNode->To1Delay = stoi(tokens[pos].value);
                    resume();
                }
                expect(",");
                if (!check_end()) 
                {
                    gateNode->To0Delay = stoi(tokens[pos].value);
                    resume();
                }
                expect(")");
            } 
            else 
            {
                if (!check_end()) 
                {
                    int delay = stoi(tokens[pos].value);
                    gateNode->To1Delay = delay;
                    gateNode->To0Delay = delay;
                    resume();
                }
            }
        } 
        else 
        {
            gateNode->To1Delay = 0;
            gateNode->To0Delay = 0;
        }

        if (!check_end() && tokens[pos].type == "IDENTIFIER") 
        {
            gateNode->gateName = tokens[pos].value;
            resume();
        } 
        else 
        {
            throw runtime_error("Expected gate name");
        }
        
        if (continue_if_wait_if_not("(")) 
        {
            if (check_end())
                throw runtime_error("Unexpected end after '('");
                
            gateNode->output = tokens[pos].value;
            resume();
            
            while (!check_end() && !exist(")")) 
            {
                continue_if_wait_if_not(",");
                if (check_end())
                    throw runtime_error("Unexpected end after ','");
                gateNode->input.push_back(tokens[pos].value);
                resume();
            }
            expect(")");
        }
    }
};
#endif
