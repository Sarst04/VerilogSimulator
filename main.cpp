#include "file.h"
#include "primitives.h"
#include "token.h"
#include "parser.h"
#include "simulationEngine.h"
#include "testbenchAnalysis.h"




void verilogSimulator(int argc, char *argv[]) 
{
    
    string code_content;
    string testbench_content;
    handleFile(argc, argv, code_content, testbench_content);

    lexicalAnalysis lexer(code_content);
    vector<lexicalAnalysis::Token> tokens = lexer.getTokens();

    vector<TimedValue> test_value = parseInput(testbench_content);

    // uncomment to see test input
    //print_testbench(test_value);


    
    // uncomment to see tokens
    //print_tokens(tokens);
    
    Parser parser(tokens);
    parseTreeNode* root = parser.parse();
    
    // uncomment to see parse tree
    // parser.printParseTree(root);

    EventScheduler sim;
    
    unordered_map<string, unique_ptr<Wire>> wire_map;


    int order = 1;
    unordered_map<int,string> input_order;
    for (auto* sig : root->wires) 
    {
        wire_map.emplace(sig->name, make_unique<Wire>(sig->name, sim, sig->Type));

        if (sig->Type == WireType::INPUT)
        {
            input_order[order] = sig->name;
            order++;
        }
    }

    vector<unique_ptr<Gate>> gate_instances;

    for (auto* g : root->gates) {
        Wire& a = *wire_map[g->input[0]];
        Wire& out = *wire_map[g->output];

        
        if (g->gateType == "not") {
            gate_instances.emplace_back(make_unique<NotGate>(g->gateName, a, out, g->To1Delay, g->To0Delay));
            continue;
        }
        
        Wire& b = *wire_map[g->input[1]];
        if (g->gateType == "and") {
            gate_instances.emplace_back(make_unique<AndGate>(g->gateName, a, b, out, g->To1Delay, g->To0Delay));
        } 
        else if (g->gateType == "or") {
            gate_instances.emplace_back(make_unique<OrGate>(g->gateName, a, b, out, g->To1Delay, g->To0Delay)); 
        }
        else if (g->gateType == "xor") {
            gate_instances.emplace_back(make_unique<XorGate>(g->gateName, a, b, out, g->To1Delay, g->To0Delay));
        }
        else if (g->gateType == "xnor") {
            gate_instances.emplace_back(make_unique<XnorGate>(g->gateName, a, b, out, g->To1Delay, g->To0Delay));
        }
        else if (g->gateType == "nand") {
            gate_instances.emplace_back(make_unique<NandGate>(g->gateName, a, b, out, g->To1Delay, g->To0Delay));
        }
        else if (g->gateType == "nor") {
            gate_instances.emplace_back(make_unique<NorGate>(g->gateName, a, b, out, g->To1Delay, g->To0Delay));
        }
        else
            throw runtime_error("Unknown gate");
        
    }

    for (auto& gate : gate_instances) {
        gate->update_delays_based_on_load();
    }

    for (const auto& test : test_value) 
    {
        sim.schedule(test.time, [&, test]() {wire_map[input_order[test.position]]->set(test.value);}, EventType::Timed);
    }

    sim.run();

    reportGatePowers(gate_instances);
    printWireFanouts(wire_map);
    
    delete root;
}


int main(int argc, char *argv[]) 
{
    try {
        verilogSimulator(argc, argv);
    } 
    catch (const invalid_argument& e) {
        cerr << "Invalid argument: " << e.what() << endl;
        return 1;
    }
    catch (const runtime_error& e) {
        cerr << "Runtime error: " << e.what() << endl;
        return 1;
    }
    catch (const exception& e) {
        cerr << "Standard exception: " << e.what() << endl;
        return 1;
    }
    catch (...) {
        cerr << "Unknown error occurred!" << endl;
        return 1;
    }

    return 0;
}