#include "primitives.h"
string readFile(const string& filePath) 
{
    ifstream file(filePath);
    if (!file.is_open()) {
        throw runtime_error("Failed to open the file: " + filePath);
    }

    stringstream buffer;
    buffer << file.rdbuf();
    file.close();
    return buffer.str();
}
void handleFile(int argc, char *argv[], string& codeContent, string& testbenchContent) 
{
    if (argc == 1) {
        codeContent = readFile("code.txt");
        testbenchContent = readFile("testbench.txt");
    } 
    else if (argc == 3) {
        codeContent = readFile(argv[1]);
        testbenchContent = readFile(argv[2]);
    }
    else {
        throw invalid_argument("Usage: ./program [code_file] [testbench_file]");
    }
}
