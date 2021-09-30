#include "diary.h"

#include <string>
#include<list>
#include <iostream>
#include <fstream>
#include <algorithm>


#include "game.h"
//#include "kill_tracker.h"
//#include "stats_tracker.h"
#include "bionics.h"
#include "calendar.h"
#include "path_info.h"
#include "output.h"
#include "cata_utility.h"
#include "avatar.h"
#include "skill.h"
#include "mtype.h"
#include "type_id.h"
#include "mutation.h"
#include "string_formatter.h"


diary_page::diary_page(std::string date, std::string text)
{
    m_date = date;
    m_text = text;
}
diary_page::diary_page() {
        
}


void diary::load_test() {
    for (int i = 0; i <= 5; i++) {
        diary::pages.push_back(new diary_page(std::to_string(i), std::to_string(i) + "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua.At vero eos et accusam et justo duo dolores et ea rebum.Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua.At vero eos et accusam et justo duo dolores et ea rebum.Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua.At vero eos et accusam et justo duo dolores et ea rebum.Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu f"));
    }
    
    
    

}

std::vector<std::string> diary::get_pages_list() {
    std::vector<std::string> result;
    for (auto n : pages) {
        result.push_back(n->m_date);

    }
    return result;
}

int diary::set_opend_page(int pagenum) {
    if (pagenum != opend_page) {
        change_list.clear();
        desc_map.clear();
    }
    if (pages.empty()) {
        opend_page = NULL;
    }
    else if (pagenum < 0) {
        opend_page = pages.size() - 1;
    }
    else {
        opend_page = pagenum % pages.size();
    }
    return opend_page;
}

int diary::get_opend_page_num() {
    return opend_page;
}


diary_page* diary::get_page_ptr(int offset) {
    if (!pages.empty()&& opend_page+offset >= 0) {
        
        return pages[opend_page+offset];
    }
    return nullptr;
}

//void diary::example_changes(std::vector<std::string>* result, diary_page* currpage, diary_page* prevpage) {
//    if (prevpage == nullptr) {}
//    else {}
//} <color_red>text</color>
// colorize
void::diary::bionic_changes(std::vector<std::string>* result) {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        if (!currpage->bionics.empty()) {
            result->push_back(_("Bionics"));
            for (const auto elem : currpage->bionics) {
                const bionic_data&b = elem.obj();
                result->push_back(b.name.translated());
            }
        }
    }
    else {
        std::vector<std::string> tempres;
        if (!currpage->bionics.empty()) {
            for (const auto elem : currpage->bionics) {
                if (std::find(prevpage->bionics.begin(), prevpage->bionics.end(), elem) == prevpage->bionics.end()) {
                    const bionic_data& b = elem.obj();
                    tempres.push_back(b.name.translated());
                }
            }
        }
        if (!tempres.empty()) {
            result->push_back(_("New Bionics: "));
            result->insert(result->end(), tempres.begin(), tempres.end());
            result->push_back(" ");
        }
        tempres.clear();
        if (!prevpage->bionics.empty()) {
            for (const auto elem : prevpage->bionics) {
                if (std::find(currpage->bionics.begin(), currpage->bionics.end(), elem) == currpage->bionics.end()) {
                    const bionic_data& b = elem.obj();                    
                    tempres.push_back(b.name.translated());
                }
            }
        }
        if (!tempres.empty()) {
            result->push_back(_("Lost Bionics: "));
            result->insert(result->end(), tempres.begin(), tempres.end());
            result->push_back(" ");
        }
        tempres.clear();
    }
}

void diary::kill_changes(std::vector<std::string>* result) {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        if (!currpage->kills.empty()) {
            result->push_back(_("Kills: "));
            for (const auto elem : currpage->kills) {
                const mtype& m = elem.first.obj();
                nc_color color = m.color;
                std::string symbol = m.sym;
                std::string nname = m.nname(elem.second);
                result->push_back(string_format("%4d ", elem.second) + colorize(symbol,
                    color) + " " + colorize(nname, c_light_gray));                    
            }
            result->push_back(" ");                
        }
        if (!currpage->npc_kills.empty()) {
            result->push_back(_("NPC Killed"));
            for (const auto& npc_name : currpage->npc_kills) {
                result->push_back(string_format("%4d ", 1) + colorize("@ " + npc_name, c_magenta));
            }
            result->push_back(" ");
        }
        
    }
    else {
        std::vector<std::string> tempres;
        if (!currpage->kills.empty()) {
            
            
            for (const auto elem : currpage->kills) {
                const mtype& m = elem.first.obj();
                nc_color color = m.color;
                std::string symbol = m.sym;
                std::string nname = m.nname(elem.second);
                int kills = elem.second;
                if (prevpage->kills.count(elem.first) > 0) {
                    const int prevkills = prevpage->kills[elem.first];
                    if (kills > prevkills) {
                        kills = kills - prevkills;
                        tempres.push_back(string_format("%4d ", kills) + colorize(symbol,
                            color) + " " + colorize(nname, c_light_gray));
                    }
                }
                else {
                    tempres.push_back(string_format("%4d ", kills) + colorize(symbol,
                        color) + " " + colorize(nname, c_light_gray));
                }
                
                
            }
            if (!tempres.empty()) {
                result->push_back(_("Kills: "));
                result->insert(result->end(), tempres.begin(), tempres.end());
                result->push_back(" ");
            }
            tempres.clear();
        }
        if (!currpage->npc_kills.empty()) {
            
            const auto prev_npc_kills = prevpage->npc_kills;
            if (!prev_npc_kills.empty()) {
                for (const auto& npc_name : currpage->npc_kills) {
                    if (npc_name != prev_npc_kills.back()) {
                        if ((std::find(prev_npc_kills.begin(), prev_npc_kills.end(), npc_name) == prev_npc_kills.end())) {
                            tempres.push_back(string_format("%4d ", 1) + colorize("@ " + npc_name, c_magenta));
                        }
                    }
                }
            }
            else
            {
                for (const auto& npc_name : currpage->npc_kills) {
                                        
                    tempres.push_back(string_format("%4d ", 1) + colorize("@ " + npc_name, c_magenta));
                }
            }
            
            if (!tempres.empty()) {
                result->push_back(_("NPC Killed: "));
                result->insert(result->end(), tempres.begin(), tempres.end());
                result->push_back(" ");
            }
            tempres.clear();
        }        
        
        
    }
}


void diary::skill_changes(std::vector<std::string>* result) {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        if (currpage->skillsL.empty()) {
            return;
        }
        else {
            
            result->push_back(_("Skills:"));
            for (auto elem : currpage->skillsL) {
                
                if (elem.second.level() > 0) {
                    result->push_back(string_format("<color_light_blue>%s: %d</color>", elem.first, elem.second.level()));
                }
            }
            result->push_back("");
        }
    }
    else
    {
        std::vector<std::string> tempres;
        for (auto elem : currpage->skillsL) {
            if (prevpage->skillsL.count(elem.first) > 0) {
                if (prevpage->skillsL[elem.first].level() != elem.second.level()) {
                    tempres.push_back(string_format(_("<color_light_blue>%s: %d -> %d</color>"), elem.first, prevpage->skillsL[elem.first].level(), elem.second.level()));
                }

            }
            
        }
        if (!tempres.empty()) {
            result->push_back(_("Skills:"));
            result->insert(result->end(), tempres.begin(), tempres.end());
            result->push_back(" ");
        }
    }
}

void diary::trait_changes(std::vector<std::string>* result) {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        if (!currpage->traits.empty()) {
            result->push_back(_("Mutations:"));
            for (const trait_id elem : currpage->traits) {
                const mutation_branch& trait = elem.obj();               
                result->push_back(colorize(trait.name(), trait.get_display_color()));
            }
            result->push_back("");
        }
    }
    else {
        if (prevpage->traits.empty() && !currpage->traits.empty()) {
            result->push_back(_("Mutations:"));
            for (const trait_id& elem : currpage->traits) {
                const mutation_branch& trait = elem.obj();
                result->push_back(colorize(trait.name(), trait.get_display_color()));
            }
            result->push_back("");
        }
        else {
            //neue traits
            std::vector<std::string> tempres;
            for (const trait_id& elem : currpage->traits) {
                
                if(std::find(prevpage->traits.begin(), prevpage->traits.end(),elem) == prevpage->traits.end()){
                    const mutation_branch& trait = elem.obj();
                    tempres.push_back(colorize(trait.name(), trait.get_display_color()));
                }
                
            }
            if (!tempres.empty()) {
                result->push_back(_("Gained Mutation:"));
                result->insert(result->end(), tempres.begin(), tempres.end());
                result->push_back(" ");
            }
            tempres.clear();
            //verlorene traids
            for (const trait_id& elem : prevpage->traits) {
                
                if (std::find(currpage->traits.begin(), currpage->traits.end(), elem) == currpage->traits.end()) {
                    const mutation_branch& trait = elem.obj();
                    tempres.push_back(colorize(trait.name(), trait.get_display_color()));
                }
            }
            if (!tempres.empty()) {
                result->push_back(_("Lost Mutation:"));
                result->insert(result->end(), tempres.begin(), tempres.end());
                result->push_back(" ");
            }
        }
    }
}

void diary::stat_changes(std::vector<std::string>* result) {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        result->push_back(_("Stats:"));
        result->push_back(string_format(_("Strength: %d"), currpage->strength));
        result->push_back(string_format(_("Dexterity: %d"), currpage->dexterity));
        result->push_back(string_format(_("Intelligence: %d"), currpage->intelligence));
        result->push_back(string_format(_("Perception: %d"), currpage->perception));
        result->push_back(" ");
    }
    else {
        std::vector<std::string> tempres;
        if (currpage->strength != prevpage->strength) tempres.push_back(string_format(_("Strength: %d -> %d"), prevpage->strength, currpage->strength));
        if (currpage->dexterity != prevpage->dexterity) tempres.push_back(string_format(_("Desxterity: %d -> %d"), prevpage->dexterity, currpage->dexterity));
        if (currpage->intelligence != prevpage->intelligence) tempres.push_back(string_format(_("Intelligence: %d -> %d"), prevpage->intelligence, currpage->intelligence));
        if (currpage->perception != prevpage->perception) tempres.push_back(string_format(_("Perception: %d -> %d"), prevpage->perception, currpage->perception));
        if (!tempres.empty()) {
            result->push_back(_("Stats:"));
            result->insert(result->end(), tempres.begin(), tempres.end());
            result->push_back(" ");
        }
    }
}

void diary::prof_changes(std::vector<std::string>* result) {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        if (!currpage->known_profs.empty()) {
            result->push_back(_("Proficiencies:"));
            for (const proficiency_id elem : currpage->known_profs) {
                const auto p = elem.obj();                
                result->push_back(p.name());
            }
            result->push_back("");
        }
        
    }
    else {
        std::vector<std::string> tempres;
        for (const proficiency_id elem : currpage->known_profs) {
            if (std::find(prevpage->known_profs.begin(), prevpage->known_profs.end(), elem) == prevpage->known_profs.end()) {
                const auto p = elem.obj();
                tempres.push_back(p.name());
            }
            
        }
        if (!tempres.empty()) {
            result->push_back(_("New Proficiencies:"));
            result->insert(result->end(), tempres.begin(), tempres.end());
            result->push_back(" ");
        }
        
    }
}

std::vector<std::string> diary::get_change_list() {
    if (!change_list.empty()) return change_list;
    if (!pages.empty()) {
        
        

        stat_changes(&change_list);
        skill_changes(&change_list);
        prof_changes(&change_list);
        trait_changes(&change_list);
        bionic_changes(&change_list);
        kill_changes(&change_list);
        
    }
    return change_list;
}
std::map<int, std::string> diary::get_desc_map() {
    if (!desc_map.empty()) {
        return desc_map;
    }
    else {
        get_change_list();
        return desc_map;
    }
    
}


std::string diary::get_page_text() {
    
    if (!pages.empty()) {
        return get_page_ptr()->m_text;
    }
    return "";
}

std::string diary::get_head_text() {
       
    if (!pages.empty()) {
        
        /*const int year = to_turns<int>(pages[position]->turn - calendar::turn_zero) / to_turns<int>
            (calendar::year_length()) + 1;*/
        const time_point prev_turn = (opend_page != 0) ? get_page_ptr(-1)->turn : calendar::turn_zero;
        const time_duration turn_diff = get_page_ptr()->turn - prev_turn;
            /*const std::string time = to_string_time_of_day();
            const int day = to_days<int>(time_past_new_year(pages[position]->turn));
            const int prevday = (position != 0)? to_days<int>(time_past_new_year(pages[position-1]->turn)): to_days<int>(time_past_new_year(calendar::turn_zero));*/
        const int days = to_days<int>(turn_diff);
        const int hours = to_hours<int>(turn_diff)%24;
        const int minutes= to_minutes<int>(turn_diff)%60;
        std::string headtext = string_format(_("Entry: %d/%d, %s, %s"),
            opend_page + 1, pages.size(),
            to_string(get_page_ptr()->turn),
            (opend_page != 0) ? string_format(_("%s%s%d minutes since last entry"),
                (days > 0) ? string_format(_("%d days, "), days) : "",
                (hours > 0) ? string_format(_("%d hours, "), hours) : "",
                 minutes) : "");
            
        return headtext;
    }
    return "";
}

diary::diary() {
    owner = get_avatar().name;
}
void diary::set_page_text( std::string text) {
    get_page_ptr()->m_text= text;
}
 
void diary::new_page() {
    diary_page* page = new diary_page();
    
    page -> m_text = "";
    page -> m_date = to_string(calendar::turn);
    page -> turn = calendar::turn;
    //
    ////game stats
    

    page -> kills = g.get()->get_kill_tracker().kills;
    page -> npc_kills = g.get()->get_kill_tracker().npc_kills;
    
    ////pimpl<stats_tracker> stats_tracker;
    //pimpl<achievements_tracker> achivement_tracker;
    ////player Stats
    avatar* u = &get_avatar();
    
    page ->male = u->male;
    page->strength = u->get_str_base();
    page->dexterity = u->get_dex_base();
    page->intelligence = u->get_int_base();
    page->perception = u->get_per_base();
    //page->addictions = u->addictions;
    
    page -> follower_ids = u->follower_ids; 
    //page -> traits = u-> my_traits;
    page->traits = u->get_mutations(false);
    
    //page -> mutations= u-> my_mutations;
    //page->magic = u->magic ;
   
    //page -> martial_arts_data = u->martial_arts_data;
    
    page-> bionics = u->get_bionics();
    
    
    

    for (auto elem : Skill::skills) {
        SkillLevel level = u->get_skill_level_object(elem.ident());
        page->skillsL.insert({ elem.name(), level });
    }
    
    page -> known_profs = u->_proficiencies->known_profs();
    page -> learning_profs = u->_proficiencies->learning_profs();
    
    //units::energy max_power_level;
    page->max_power_level = u->get_max_power_level();
    diary::pages.push_back(page);
}

void diary::delete_page() {
    if (opend_page < pages.size()) {
        pages.erase(pages.begin() + opend_page );
    }
    
}

void diary::export_to_txt() {
    std::ofstream myfile;
    myfile.open(PATH_INFO::world_base_save_path() + "\\"+owner +"s_diary.txt");
    //myfile << "Writing this to a file.\n";
    //std::vector<diary_page*>::iterator ptr;
    for (int i = 0; i < pages.size(); i++) {
        set_opend_page(i);
        const diary_page page = *get_page_ptr();
        myfile << get_head_text() + "\n\n";
        for (auto str : this->get_change_list()) {
            myfile << remove_color_tags(str) + "\n";
        }
        auto folded_text = foldstring(page.m_text, 50);
        for (int i = 0; i < folded_text.size(); i++) {
            myfile << folded_text[i] + "\n";
        }
        myfile <<  "\n\n\n";
    }
    myfile.close();
}
void diary::serialize(std::ostream& fout) {
    JsonOut jout(fout,true);
    jout.start_object();
    jout.member("owner", owner);
    jout.member("pages");
    jout.start_array();
    for (auto n : pages) {
        jout.start_object();
        jout.member("date", n->m_date);
        jout.member("text", n->m_text);
        jout.member("turn", n->turn);
        jout.member("kills", n->kills);
        jout.member("npc_kills", n->npc_kills);
        jout.member("male", n->male);
        //jout.member("addictions", n->addictions);
        jout.member("str", n->strength);
        jout.member("dex", n->dexterity);
        jout.member("int", n->intelligence);
        jout.member("per", n->perception);
        jout.member("follower_ids", n->follower_ids);
        jout.member("traits", n->traits);
        jout.member("bionics", n->bionics);
        //jout.member("mutations", n->mutations);
        jout.member("skillsL", n->skillsL);
            /*jout.start_array();
                for (auto elem : n->skillsL) {
                    jout.member(elem.first, elem.second);
                }
            jout.end_array();*/        
        jout.member("known_profs", n->known_profs);
        jout.member("learning_profs", n->learning_profs);
        jout.member("max_power_level", n->max_power_level);
        jout.end_object();
    }
    jout.end_array();
    jout.end_object();
}
void diary::serialize() {
    std::string path = PATH_INFO::world_base_save_path() +  "\\"+ owner+"_diary.json"; 
    write_to_file(path, [&](std::ostream& fout) {
        serialize(fout);
        }, _("diary data"));
}
void diary::deserialize(std::istream& fin) {
    
    
    try {
        JsonIn jin(fin);
        auto data = jin.get_object();
        data.read("owner", owner);
        pages.clear();
        //auto arr = data.get_array("pages");
        for (JsonObject elem : data.get_array("pages")) {
            diary_page* page = new diary_page();
            page->m_date = elem.get_string("date");
            page->m_text = elem.get_string("text");
            elem.read("turn", page->turn);
            elem.read("kills", page->kills);
            elem.read("npc_kills", page->npc_kills);
            elem.read("male", page->male);
            //elem.read("addictions", page->addictions);
            elem.read("str", page->strength);
            elem.read("dex", page->dexterity);
            elem.read("int", page->intelligence);
            elem.read("per", page->perception);
            elem.read("follower_ids", page->follower_ids);
            elem.read("traits", page->traits);
            elem.read("bionics", page->bionics);
            //elem.read("mutations", page.mutations);
            elem.read("skillsL", page->skillsL);
            
            /*for (JsonObject s : elem.get_array("skillsL")) {
                std::string name = s.get_member();
                
            }*/
            elem.read("known_profs", page->known_profs);
            elem.read("learning_profs", page->learning_profs);
            elem.read("max_power_level", page->max_power_level);
            
            pages.push_back(page);
        }
    }catch (const JsonError& e) {
    //debugmsg("error loading %s: %s", SAVE_MASTER, e.c_str());
    }
    

}
void diary::deserialize() {
    std::string path = PATH_INFO::world_base_save_path() + "\\"+ owner + "_diary.json";

    read_from_file(path, [&](std::istream& fin) {
        deserialize(fin);
        });
}

/**methode, die eine zwei pages bekommt und einen vector string zurück gibt mit den änderungen*/

/**methode für jede subkathegorie an informationen */


