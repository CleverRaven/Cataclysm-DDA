#include <vector>
#include <string>
#include <stack>
#include <condition_variable>
#include <string>
#include <list>

#include "json.h"
#include "input.h"



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

    std::stack<std::string> ctxt;
    std::list<input_context *> context_stack; // current input context that corresponds with keybinds
    std::stack<std::string> mctxt; // current input context, for menus
    


    // Everything that goes in the output goes here
    void serialize(JsonOut &jsout) const;

    void write_out(std::string json);

private:
    // Blank constructor
    gsi() {}



};

class gsi_thread
{
public:
    std::condition_variable gsi_update;
    static void worker();  // Start the writeout thread
};
