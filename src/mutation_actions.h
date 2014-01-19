#ifndef _MUTATION_ACTIONS_H_
#define _MUTATION_ACTIONS_H_
#include "json.h"
#include <string>
#include <map>

void draw_exam_window(WINDOW *win, int border_line, bool examination);

class mut_action_data {
public:
     mut_action_data(std::string new_name, bool new_activated, std::string new_description);

     std::string name;
     bool activated; // If true, then the below function only happens when
                     // activated; otherwise, it happens every turn
     std::string description;
};


struct mut_action: public JsonSerializer, public JsonDeserializer
{
    std::string id;
    char invlet;
    bool activated;
    mut_action() : id("shout") {
        invlet = 'a';
        activated = false;
    }
    mut_action(std::string pid, char pinvlet) : id(pid) {
        id = pid;
        invlet = pinvlet;
        activated = false;
    };
    
    using JsonSerializer::serialize;
    void serialize(JsonOut &json) const {
        json.start_object();
        json.member("id", id);
        json.member("invlet", (int)invlet);
        json.member("activated", activated);
        json.end_object();
    }
    
    using JsonDeserializer::deserialize;
    void deserialize(JsonIn &jsin) {
        JsonObject jo = jsin.get_object();
        id = jo.get_string("id");
        invlet = jo.get_int("invlet");
        activated = jo.get_bool("activated");
    }
};

extern std::map<std::string, mut_action_data *> mut_actions;

void load_mut_action(JsonObject &jsobj); // load actions from JSON

#endif
