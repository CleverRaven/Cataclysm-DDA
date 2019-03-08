#include <vector>
#include <string>
#include <stack>
#include <condition_variable>
#include <string>

#include "json.h"

class gsi : public JsonSerializer
{
public:

    std::condition_variable gsi_update;

    // Singleton setup
    static gsi& get()
    {
        static gsi instance; // Guaranteed to be destroyed.
                                        // Instantiated on first use.
        return instance;
    }
    gsi(gsi const&) = delete;
    void operator=(gsi const&) = delete;

    std::vector<std::vector<std::string>> vWorldHotkeys;
    std::vector<std::vector<std::string>> vSettingsHotkeys;
    std::vector<std::vector<std::string>> vMenuHotkeys; // hotkeys for the vMenuItems

    std::vector<std::vector<std::string>> vNewGameHotkeys;

    std::list<input_context *> context_stack; // current input context that corresponds with keybinds
    std::stack<std::string> mctxt; // current input context, for menus
    
    void gsi_thread();  // Start the writeout thread


    // Everything that goes in the output goes here
    void serialize(JsonOut &jsout) const 
    {
        jsout.start_object();
        jsout.member("provdier");
        jsout.start_object();
        jsout.member("name", "cataclysm");
        jsout.member("appid", -1);
        jsout.end_object();

        jsout.member("keybinds");
        jsout.start_object();
        jsout.member("input_context", context_stack.front);
        jsout.member("menu_context", mctxt);
        jsout.member("localization");
        jsout.start_object();
        jsout.member("main_world", vWorldHotkeys);
        jsout.member("main_settings", vSettingsHotkeys);
        jsout.member("main_menu", vMenuHotkeys);
        jsout.member("main_newgame", vNewGameHotkeys);
        jsout.end_object();
        jsout.end_object();

        //jsout.start_array();
        //jsout.write(x);
        //jsout.write(y);
        //jsout.end_array();

        jsout.end_object();
    }

private:
    // Blank constructor
    gsi() {}

    void write_out();

};
