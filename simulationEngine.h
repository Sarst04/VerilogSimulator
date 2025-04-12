#include "primitives.h"

constexpr char LOGIC_0 = '0';
constexpr char LOGIC_1 = '1';
constexpr char LOGIC_X = 'x';
constexpr char LOGIC_Z = 'z';

using Time = uint64_t;

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
    void set(char new_value);
    void attach(Gate* gate);
};

class Gate {
public:
    virtual ~Gate() = default;
    virtual void evaluate() = 0;
    virtual PowerStats get_power_stats() const = 0;
    virtual string get_name() const = 0;
        
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

    
void reportGatePowers(const vector<unique_ptr<Gate>>& gates) 
{
    
    double total_dynamic = 0.0;
    double total_static = 0.0;
    double total_power = 0.0;
    

    double most_power=0;
    string most_power_name;
    cout << "----- Gate Power Report -----" << endl;
    for (const auto& gate : gates) 
    {
        string gateName = gate->get_name();
        PowerStats stats = gate->get_power_stats();
        double dynamic_power = stats.dynamic_power;
        double static_power  = stats.static_power;
        if (dynamic_power > most_power)
        {
            most_power = dynamic_power;
            most_power_name = gateName;
        }
        cout << "Gate \"" << gateName << "\": "
             << "Dynamic Power = " << dynamic_power << " W, "
             << "Static Power = " << static_power << " W, "
             << "Total Power = " << dynamic_power + static_power << " W" << endl;
        total_dynamic += dynamic_power;
        total_static += static_power;
        total_power +=  dynamic_power + static_power ;
    }
    cout << "----- Total Power Report -----" << endl;
    cout <<  "Dynamic Power = " << total_dynamic << ".\n";
    cout <<  "Static Power = " << total_static << ".\n";
    cout <<  "Total Power = " << total_power << ".\n";
    cout << "Gate " << most_power_name << "use most power( " << most_power<<" )"<<".\n";
}
