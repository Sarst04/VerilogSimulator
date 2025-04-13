#ifndef PRIMITIVE_H
#define PRIMITIVE_H



#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"

#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept> 
#include <string>
#include <vector>
#include <iomanip>
#include <memory>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <functional>
#include <tuple>
#include <algorithm>


using namespace std;

constexpr double GATE_CAPACITANCE = 1;
constexpr double SUPPLY_VOLTAGE = 1.8;
constexpr double STATIC_POWER_PER_GATE = 0.000001;

constexpr double LOAD_DEPENDENT_DELAY_FACTOR = 0.2*0;
constexpr double AGING_EFFECT = 1.3-0.3;

const double HIGH_POWER_THRESHOLD = 1.5;
const size_t HIGH_FANOUT_THRESHOLD = 3;

#endif // PRIMITIVE_H
