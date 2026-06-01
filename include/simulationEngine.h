#include "primitives.h"

constexpr char LOGIC_0 = '0';
constexpr char LOGIC_1 = '1';
constexpr char LOGIC_X = 'x';
constexpr char LOGIC_Z = 'z';

using Time = double;

enum class EventType { Delta, Timed };

struct PowerStats {
    double dynamic_power = 0.0;
    double static_power = 0.0;
};

class EventScheduler;


class Wire {
public:
    string name;
    char value = LOGIC_X;
    vector<class Gate*> listeners;
    EventScheduler& scheduler;
    WireType type;

    Wire(const string& name, EventScheduler& scheduler, WireType type);
    size_t get_listeners_quantity();
    void set(char new_value);
    void attach(Gate* gate);

};

class Gate {
public:
    virtual ~Gate() = default;
    virtual void evaluate() = 0;
    virtual PowerStats get_power_stats() const = 0;
    virtual string get_name() const = 0;
    virtual void update_delays_based_on_load() = 0;
        
protected:
    PowerStats calculate_power(char old_val, char new_val) const {
        PowerStats stats;
        if (old_val != new_val) {
            stats.dynamic_power = 0.5 * GATE_CAPACITANCE * SUPPLY_VOLTAGE * SUPPLY_VOLTAGE;
        }
        stats.static_power = STATIC_POWER_PER_GATE;
        return stats;
    }

    void update_power_stats(PowerStats& current_stats, char old_val, char new_val) const {
        PowerStats stats = calculate_power(old_val, new_val);
        current_stats.dynamic_power += stats.dynamic_power;
        current_stats.static_power += stats.static_power;
    }
    Time calculate_load_delay(Time base_delay, size_t fan_out) const {
        return (base_delay * static_cast<Time>(fan_out * LOAD_DEPENDENT_DELAY_FACTOR)+1)*AGING_EFFECT;
    }
};

class Event {
    Time time_;
    int delta_order;
    EventType type_event;
    function<void()> callback;

public:
    Event(Time t, int order, EventType type, function<void()> cb)
        : time_(t), 
        delta_order(order), 
        type_event(type), 
        callback(cb) {}

    bool operator>(const Event& other) const {
        if (time_ != other.time_) 
            return time_ > other.time_;
        return delta_order > other.delta_order;
    }

    void execute() const { callback(); }
    Time time() const { return time_; }
    EventType type() const { return type_event; }
};

class EventScheduler {
    Time current_time = 0;
    int delta_count = 0;

    priority_queue<Event, vector<Event>, greater<> > event_queue;

public:

    void schedule(Time t, std::function<void()> callback, EventType type) 
    {
        int order = (type == EventType::Delta) ? delta_count++ : 0;
        event_queue.emplace(t, order, type, callback);
    }

    void run() 
    {
        while (!event_queue.empty()) 
        {
            Event event = event_queue.top();
            event_queue.pop();

            if (event.time() > current_time) 
            {
                current_time = event.time();
                delta_count = 0;
            }

            event.execute();
        }
    }

    Time now() const { return current_time; }
};



Wire::Wire(const std::string& name, EventScheduler& scheduler, WireType type)
    : name(name), 
    scheduler(scheduler),
    type(type) {}

void Wire::set(char new_value) {
    if (value != new_value) 
    {
        //print outputs here in code
        if (type == WireType::OUTPUT)
            cout << "[" << scheduler.now() << "ps] " << name << ": " 
                << value << " -> " << new_value << endl;

        value = new_value;
        for (auto* gate : listeners)
        {
            scheduler.schedule(scheduler.now(), [gate] {
                gate->evaluate();
            }, EventType::Delta);
        }
    }
}

void Wire::attach(Gate* gate) {
    listeners.push_back(gate);
}
size_t Wire::get_listeners_quantity() { 
    if (type==WireType::OUTPUT)
        return 1;
    return listeners.size(); 
}

class AndGate : public Gate {
    Wire& input_a_;
    Wire& input_b_;
    Wire& output_;
    Time delay_rise_;
    Time delay_fall_;
    mutable PowerStats power_stats_;
    string gate_name;

public:
    AndGate(const string& name, Wire& a, Wire& b, Wire& out, Time rise_delay, Time fall_delay)
        : input_a_(a), input_b_(b), output_(out),
          delay_rise_(rise_delay), delay_fall_(fall_delay), 
          gate_name(name){
        a.attach(this);
        b.attach(this);
    }
    void update_delays_based_on_load() override {
        size_t fan_out = output_.get_listeners_quantity();
        delay_rise_ = calculate_load_delay(delay_rise_, fan_out);
        delay_fall_ = calculate_load_delay(delay_fall_, fan_out);
    }
    string get_name() const override { return gate_name; }
    void evaluate() override 
    {
        char a_val = input_a_.value;
        char b_val = input_b_.value;
        char result;

        if (a_val == LOGIC_0 || b_val == LOGIC_0) 
            result = LOGIC_0;
        else if (a_val == LOGIC_1 && b_val == LOGIC_1)
            result = LOGIC_1;
        else
            result = LOGIC_X;
        
        Time delay = (result == LOGIC_1) ? delay_rise_ : delay_fall_;
        if (result == LOGIC_X) delay = 0;

        
        output_.scheduler.schedule(output_.scheduler.now() + delay, [this, result] {
            char old_val = output_.value;
            output_.set(result);
            update_power_stats(power_stats_, old_val, result);
        }, EventType::Timed);
    }
    
    PowerStats get_power_stats() const override {
        return power_stats_;
    }
};

class OrGate : public Gate {
    Wire& input_a_;
    Wire& input_b_;
    Wire& output_;
    Time delay_rise_;
    Time delay_fall_;
    mutable PowerStats power_stats_;
    string gate_name;

public:
    OrGate(const string& name, Wire& a, Wire& b, Wire& out, Time rise_delay, Time fall_delay)
        : input_a_(a), input_b_(b), output_(out),
          delay_rise_(rise_delay), delay_fall_(fall_delay), 
          gate_name(name){
        a.attach(this);
        b.attach(this);
    }
    void update_delays_based_on_load() override {
        size_t fan_out = output_.get_listeners_quantity();
        delay_rise_ = calculate_load_delay(delay_rise_, fan_out);
        delay_fall_ = calculate_load_delay(delay_fall_, fan_out);
    }
    string get_name() const override { return gate_name; }
    void evaluate() override 
    {
        char a_val = input_a_.value;
        char b_val = input_b_.value;
        char result;

        if (a_val == LOGIC_1 || b_val == LOGIC_1) 
            result = LOGIC_1;
        else if (a_val == LOGIC_0 && b_val == LOGIC_0)
            result = LOGIC_0;
        else
            result = LOGIC_X;

        Time delay = (result == LOGIC_1) ? delay_rise_ : delay_fall_;
        if (result == LOGIC_X) delay = 0;   

        output_.scheduler.schedule(output_.scheduler.now() + delay, [this, result] {
            char old_val = output_.value;
            output_.set(result);
            update_power_stats(power_stats_, old_val, result);
        }, EventType::Timed);
    }
    
    PowerStats get_power_stats() const override {
        return power_stats_;
    }
};

class NotGate : public Gate {
    Wire& input_a_;
    Wire& output_;
    Time delay_rise_;
    Time delay_fall_;
    mutable PowerStats power_stats_;
    string gate_name;

public:
    NotGate(const string& name, Wire& a, Wire& out, Time rise_delay, Time fall_delay)
        : input_a_(a), output_(out),
          delay_rise_(rise_delay), delay_fall_(fall_delay), 
          gate_name(name){
        a.attach(this);
    }
    void update_delays_based_on_load() override {
        size_t fan_out = output_.get_listeners_quantity();
        delay_rise_ = calculate_load_delay(delay_rise_, fan_out);
        delay_fall_ = calculate_load_delay(delay_fall_, fan_out);
    }

    string get_name() const override { return gate_name; }
    void evaluate() override 
    {
        char a_val = input_a_.value;
        char result;

        if (a_val == LOGIC_1) 
            result = LOGIC_0;
        else if (a_val == LOGIC_0)
            result = LOGIC_1;
        else
            result = LOGIC_X;

        Time delay = (result == LOGIC_1) ? delay_rise_ : delay_fall_;
        if (result == LOGIC_X) delay = 0;   

        output_.scheduler.schedule(output_.scheduler.now() + delay, [this, result] {
            char old_val = output_.value;
            output_.set(result);
            update_power_stats(power_stats_, old_val, result);
        }, EventType::Timed);
    }
    
    PowerStats get_power_stats() const override {
        return power_stats_;
    }
};

class XorGate : public Gate {
    Wire& input_a_;
    Wire& input_b_;
    Wire& output_;
    Time delay_rise_;
    Time delay_fall_;
    mutable PowerStats power_stats_;
    string gate_name;

public:
    XorGate(const string& name, Wire& a, Wire& b, Wire& out, Time rise_delay, Time fall_delay)
        : input_a_(a), input_b_(b), output_(out),
          delay_rise_(rise_delay), delay_fall_(fall_delay), 
          gate_name(name){
        a.attach(this);
        b.attach(this);
    }
    void update_delays_based_on_load() override {
        size_t fan_out = output_.get_listeners_quantity();
        delay_rise_ = calculate_load_delay(delay_rise_, fan_out);
        delay_fall_ = calculate_load_delay(delay_fall_, fan_out);
    }

    string get_name() const override { return gate_name; }

    void evaluate() override 
    {
        char a_val = input_a_.value;
        char b_val = input_b_.value;
        char result;

        if (a_val == LOGIC_X || b_val == LOGIC_X) 
            result = LOGIC_X;
        else if (a_val != b_val)
            result = LOGIC_1;
        else
            result = LOGIC_0;

        Time delay = (result == LOGIC_1) ? delay_rise_ : delay_fall_;
        if (result == LOGIC_X) delay = 0;    

        output_.scheduler.schedule(output_.scheduler.now() + delay, [this, result] {
            char old_val = output_.value;
            output_.set(result);
            update_power_stats(power_stats_, old_val, result);
        }, EventType::Timed);
    }
    
    PowerStats get_power_stats() const override {
        return power_stats_;
    }
};
class XnorGate : public Gate {
    Wire& input_a_;
    Wire& input_b_;
    Wire& output_;
    Time delay_rise_;
    Time delay_fall_;
    mutable PowerStats power_stats_;
    string gate_name;

public:
    XnorGate(const string& name, Wire& a, Wire& b, Wire& out, Time rise_delay, Time fall_delay)
        : input_a_(a), input_b_(b), output_(out),
          delay_rise_(rise_delay), delay_fall_(fall_delay), 
          gate_name(name){
        a.attach(this);
        b.attach(this);
    }
    void update_delays_based_on_load() override {
        size_t fan_out = output_.get_listeners_quantity();
        delay_rise_ = calculate_load_delay(delay_rise_, fan_out);
        delay_fall_ = calculate_load_delay(delay_fall_, fan_out);
    }

    string get_name() const override { return gate_name; }
    void evaluate() override 
    {
        char a_val = input_a_.value;
        char b_val = input_b_.value;
        char result;

        if (a_val == LOGIC_X || b_val == LOGIC_X) 
            result = LOGIC_X;
        else if (a_val == b_val)
            result = LOGIC_1;
        else
            result = LOGIC_0;

        Time delay = (result == LOGIC_1) ? delay_rise_ : delay_fall_;
        if (result == LOGIC_X) delay = 0;    

        output_.scheduler.schedule(output_.scheduler.now() + delay, [this, result] {
            char old_val = output_.value;
            output_.set(result);
            update_power_stats(power_stats_, old_val, result);
        }, EventType::Timed);
    }
    
    PowerStats get_power_stats() const override {
        return power_stats_;
    }
};
class NandGate : public Gate {
    Wire& input_a_;
    Wire& input_b_;
    Wire& output_;
    Time delay_rise_;
    Time delay_fall_;
    mutable PowerStats power_stats_;
    string gate_name;

public:
    NandGate(const string& name, Wire& a, Wire& b, Wire& out, Time rise_delay, Time fall_delay)
        : input_a_(a), input_b_(b), output_(out),
          delay_rise_(rise_delay), delay_fall_(fall_delay), 
          gate_name(name){
        a.attach(this);
        b.attach(this);
    }
    void update_delays_based_on_load() override {
        size_t fan_out = output_.get_listeners_quantity();
        delay_rise_ = calculate_load_delay(delay_rise_, fan_out);
        delay_fall_ = calculate_load_delay(delay_fall_, fan_out);
    }
    string get_name() const override { return gate_name; }
    void evaluate() override 
    {
        char a_val = input_a_.value;
        char b_val = input_b_.value;
        char result;

        if (a_val == LOGIC_0 || b_val == LOGIC_0) 
            result = LOGIC_1;
        else if (a_val == LOGIC_1 && b_val == LOGIC_1)
            result = LOGIC_0;
        else
            result = LOGIC_X;

        Time delay = (result == LOGIC_1) ? delay_rise_ : delay_fall_;
        if (result == LOGIC_X) delay = 0;   

        output_.scheduler.schedule(output_.scheduler.now() + delay, [this, result] {
            char old_val = output_.value;
            output_.set(result);
            update_power_stats(power_stats_, old_val, result);
        }, EventType::Timed);
    }
    
    PowerStats get_power_stats() const override {
        return power_stats_;
    }
};

class NorGate : public Gate {
    Wire& input_a_;
    Wire& input_b_;
    Wire& output_;
    Time delay_rise_;
    Time delay_fall_;
    mutable PowerStats power_stats_;
    string gate_name;

public:
    NorGate(const string& name, Wire& a, Wire& b, Wire& out, Time rise_delay, Time fall_delay)
        : input_a_(a), input_b_(b), output_(out),
          delay_rise_(rise_delay), delay_fall_(fall_delay), 
          gate_name(name){
        a.attach(this);
        b.attach(this);
    }
    void update_delays_based_on_load() override {
        size_t fan_out = output_.get_listeners_quantity();
        delay_rise_ = calculate_load_delay(delay_rise_, fan_out);
        delay_fall_ = calculate_load_delay(delay_fall_, fan_out);
    }
    string get_name() const override { return gate_name; }
    void evaluate() override 
    {
        char a_val = input_a_.value;
        char b_val = input_b_.value;
        char result;

        if (a_val == LOGIC_1 || b_val == LOGIC_1) 
            result = LOGIC_0;
        else if (a_val == LOGIC_0 && b_val == LOGIC_0)
            result = LOGIC_1;
        else
            result = LOGIC_X;

        Time delay = (result == LOGIC_1) ? delay_rise_ : delay_fall_;
        if (result == LOGIC_X) delay = 0;    

        output_.scheduler.schedule(output_.scheduler.now() + delay, [this, result] {
            char old_val = output_.value;
            output_.set(result);
            update_power_stats(power_stats_, old_val, result);
        }, EventType::Timed);
    }
    
    PowerStats get_power_stats() const override {
        return power_stats_;
    }
};



struct GatePowerData {
    string name;
    double dynamic_power;
    double static_power;
    double total_power;
    
    bool operator<(const GatePowerData& other) const {
        return total_power > other.total_power;
    }
};

void reportGatePowers(const vector<unique_ptr<Gate>>& gates) 
{
    vector<GatePowerData> power_data;
    double total_dynamic = 0.0;
    double total_static = 0.0;
    double total_power = 0.0;

    for (const auto& gate : gates) 
    {
        PowerStats stats = gate->get_power_stats();
        GatePowerData data{
            gate->get_name(),
            stats.dynamic_power,
            stats.static_power,
            stats.dynamic_power + stats.static_power
        };
        
        total_dynamic += data.dynamic_power;
        total_static += data.static_power;
        total_power += data.total_power;

        power_data.push_back(data);
        
    }

    sort(power_data.begin(), power_data.end());

    cout << "\n┌──────────────────────┬──────────────────┬──────────────────┬──────────────────┐\n";
    cout << "│" << COLOR_CYAN << "       Gate Name      " << COLOR_RESET << "│" 
         << COLOR_CYAN << "  Dynamic Power   " << COLOR_RESET << "│" 
         << COLOR_CYAN << "   Static Power   " << COLOR_RESET << "│" 
         << COLOR_CYAN << "   Total Power    " << COLOR_RESET << "│\n";
    cout << "├──────────────────────┼──────────────────┼──────────────────┼──────────────────┤\n";
    
    for (const auto& data : power_data) 
    {
        const char* name_color = (data.dynamic_power > HIGH_POWER_THRESHOLD) ? COLOR_RED : "";
        const char* dyn_color = (data.dynamic_power > HIGH_POWER_THRESHOLD) ? COLOR_RED : "";
        const char* total_color = (data.total_power > HIGH_POWER_THRESHOLD) ? COLOR_YELLOW : "";
        
        printf("│ %s%-20s%s │ %s%16.2e%s │ %16.2e │ %s%16.2e%s │\n",
                   name_color, data.name.c_str(), COLOR_RESET,
                   dyn_color, data.dynamic_power, COLOR_RESET,
                   data.static_power,
                   total_color, data.total_power, COLOR_RESET);
    }
    
    cout << "└──────────────────────┴──────────────────┴──────────────────┴──────────────────┘\n";

    cout << "\n" << COLOR_GREEN << "SUMMARY:" << COLOR_RESET << "\n";
    cout << "Total Dynamic Power: " << COLOR_YELLOW << total_dynamic << " W" << COLOR_RESET << "\n";
    cout << "Total Static Power:  " << total_static << " W\n";
    cout << "Total Power:         " << COLOR_YELLOW << total_power << " W" << COLOR_RESET << "\n";
}

void printWireFanouts(const std::unordered_map<std::string, std::unique_ptr<Wire>>& wire_map) 
{
    vector<pair<string, size_t>> fanouts;
    size_t max_fanout = 0;
    string max_fanout_wire;
    
    for (const auto& [name, wire] : wire_map) 
    {
        size_t count = wire->get_listeners_quantity();
        fanouts.emplace_back(name, count);
        
        if (count > max_fanout) 
        {
            max_fanout = count;
            max_fanout_wire = name;
        }
    }
    sort(fanouts.begin(), fanouts.end(),[](const auto& a , const auto& b){return a.second > b.second;});
    
    std::cout << "\n┌──────────────────────┬──────────────────┐\n";
    std::cout << "│" << COLOR_CYAN << "      Wire Name       " << COLOR_RESET << "│" 
              << COLOR_CYAN << "  Fanout Count    " << COLOR_RESET << "│\n";
    std::cout << "├──────────────────────┼──────────────────┤\n";
    
    for (const auto& [name, count] : fanouts) 
    {
        const char* color = (count > HIGH_FANOUT_THRESHOLD) ? COLOR_MAGENTA : "";
        printf("│ %s%-20s%s │ %s%16zu%s │\n",
                   color, name.c_str(), COLOR_RESET,
                   color, count, COLOR_RESET);
    }
    
    cout << "└──────────────────────┴──────────────────┘\n";
    cout << "\n" << COLOR_GREEN << "SUMMARY:" << COLOR_RESET << "\n";
    cout << "Total Wires: " << wire_map.size() << "\n";
}
