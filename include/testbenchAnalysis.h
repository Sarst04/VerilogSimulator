#include "primitives.h"
struct TimedValue {
    int time;
    char value;
    int position;
};

vector<TimedValue> parseInput(const string& input) {
    vector<TimedValue> result;
    istringstream iss(input);
    string line;
    
    while (getline(iss, line)) {
        if (line.empty()) continue;
        
        size_t hash_pos = line.find('#');
        size_t space_pos = line.find(' ');
        
        if (hash_pos != string::npos && space_pos != string::npos && space_pos > hash_pos) {
            string time_str = line.substr(hash_pos + 1, space_pos - hash_pos - 1);
            int time = stoi(time_str);
            string values = line.substr(space_pos + 1);
            
            for (int i = 0; i < values.size(); i++) {
                char val = values[i];
                if (val == '1' || val == '0' || val == 'x' || val == 'z') {
                    result.push_back({time, val, i+1});
                }
            }
        }
    }
    return result;
}

void print_testbench(vector<TimedValue> test_value)
{
    cout << "\nTestBench :\n";
    cout << "----------------\n";
    for (const auto& entry : test_value) {
        cout << "Time " << entry.time << ": ";
        cout << "Input " << entry.position << " = " << entry.value << "\n";
    }
}
