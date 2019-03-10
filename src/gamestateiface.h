#include <vector>
#include <string>
#include <stack>
#include <condition_variable>
#include <string>
#include <list>
#include <mutex>

#include "json.h"
#include "color.h"
#include "pldata.h"
#include "bodypart.h"



class gsi : public JsonSerializer
{
public:

    std::condition_variable gsi_update;
    std::condition_variable gsi_writer;
    // Singleton setup
    static gsi& get()
    {
        static gsi instance; // Guaranteed to be destroyed.
                                        // Instantiated on first use.
        return instance;
    }
    gsi(gsi const&) = delete;
    void operator=(gsi const&) = delete;

    std::stack<std::string> ctxt;  // current input context
    std::stack<std::string> mctxt; // current input context, for menus
    
    std::list<long> invlets;       // inventory letters in use
    std::list<nc_color> invlets_c; // inventory letters' corresponding colors

    bool is_self_aware;

    void update_hunger(int hunger, int starvation);

    void update_thirst(int thirst);

    void update_fatigue(int fatigue);

    void update_body(std::array<int, num_hp_parts> hp_cur, std::array<int, num_hp_parts> hp_max, 
        std::array<nc_color, num_hp_parts> bp_status, std::array<float, num_hp_parts> splints);

    void update_temp(std::array<int, num_bp> temp_cur, std::array<int, num_bp> temp_conv);

    int stamina, stamina_max;
    int power_level, max_power_level;
    int pain;
    int morale;
    int safe_mode;

    // Everything that goes in the output goes here
    void serialize(JsonOut &jsout) const;

    void write_out();

private:
    // Blank constructor
    gsi() 
    {
        ctxt.push("default");
        mctxt.push("default");
    }

    int hunger_level, thirst_level, fatigue_level;
    std::array<int, num_hp_parts> hp_cur_level, hp_max_level;
    std::array<int, num_hp_parts> limb_state;
    std::array<float, num_hp_parts> splint_state;
    int temp_level, temp_change;
};

class gsi_thread
{
public:
    std::condition_variable gsi_update;
    static void worker();  // Start the writeout thread
    static void prep_out();
};
