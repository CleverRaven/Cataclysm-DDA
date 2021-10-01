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
#include "mission.h"
#include "avatar.h"
#include "skill.h"
#include "mtype.h"
#include "type_id.h"
#include "mutation.h"
#include "string_formatter.h"



namespace {

}
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

void diary::add_to_change_list(std::string entry, std::string desc) {
    if(desc != "") desc_map[change_list.size()] = desc;
    change_list.push_back(entry);
}
//void diary::example_changes(std::vector<std::string>* result, diary_page* currpage, diary_page* prevpage) {
//    if (prevpage == nullptr) {}
//    else {}
//} <color_red>text</color>
// colorize

/*auto spell = u->magic->get_spell(id);
        spell.name();
        spell.description();*/
void diary::spell_changes() {
    avatar* u = &get_avatar();
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr) return;
    if (prevpage == nullptr) {
        if (!currpage->known_spells.empty()) {
            add_to_change_list(_("Known spells:"));
            for (const auto elem : currpage->known_spells) {
                const spell s = u->magic->get_spell(elem.first);
                add_to_change_list(string_format(_("%s: %d"), s.name(), elem.second), s.description());
            }
            add_to_change_list(" ");
        }
    }
    else {
        if (!currpage->known_spells.empty()) {
            bool flag = true;
            for (const auto elem : currpage->known_spells) {
                if (prevpage->known_spells.find(elem.first) != prevpage->known_spells.end()) {
                    const int prevlvl = prevpage->known_spells[elem.first];
                    if (elem.second != prevlvl) {
                        if (flag) {
                            add_to_change_list(_("Improved/new spells: "));
                            flag = false;
                        }
                        const spell s = u->magic->get_spell(elem.first);
                        add_to_change_list(string_format(_("%s: %d -> %d"), s.name(),prevlvl, elem.second), s.description());
                    }
                }
                else {
                    if (flag) {
                        add_to_change_list(_("Improved/new spells: "));
                        flag = false;
                    }
                    const spell s = u->magic->get_spell(elem.first);
                    add_to_change_list(string_format(_("%s: %d"), s.name(), elem.second), s.description());
                }
            }
            if (!flag) add_to_change_list(" ");
        }
        
    }
}



void diary::mission_changes() {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr) return;
    if (prevpage == nullptr) {
        auto add_missions = [&](const std::string name, const std::vector<int>* missions) {
            if (!missions->empty()) {
                bool flag = true;

                for (const int uid : *missions) {
                    const auto miss = mission::find(uid);
                    if (miss != nullptr) {
                        if (flag) {
                            add_to_change_list(name);
                            flag = false;
                        }
                        add_to_change_list(miss->name(), miss->get_description());
                    }
                }
                if (!flag) add_to_change_list(" ");
            }
        };
        add_missions(_("Active missions:"), &currpage->mission_active);
        add_missions(_("Completed missions:"), &currpage->mission_completed);
        add_missions(_("Faild missions:"), &currpage->mission_faild);
       
    }
    else {
        auto add_missions = [&](const std::string name, const std::vector<int>* missions, const std::vector<int>* prevmissions) {
            bool flag = true;
            for (const int uid : *missions) {
                if (std::find(prevmissions->begin(), prevmissions->end(),uid) == prevmissions->end()) {
                    const auto miss = mission::find(uid);
                    if (miss != nullptr) {
                        if (flag) {
                            add_to_change_list(name);
                            flag = false;
                        }
                        add_to_change_list(miss->name(), miss->get_description());
                    }
                    
                }
                if (!flag) add_to_change_list(" ");
            }
        };
        add_missions(_("New missions:"), &currpage->mission_active, &prevpage->mission_active);
        add_missions(_("New completed missions:"), &currpage->mission_completed, &prevpage->mission_completed);
        add_missions(_("New faild:"), &currpage->mission_faild, &prevpage->mission_faild);

    }
}

void diary::bionic_changes() {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr) return;
    if (prevpage == nullptr) {
        if (!currpage->bionics.empty()) {
            add_to_change_list(_("Bionics"));
            for (const auto elem : currpage->bionics) {
                const bionic_data&b = elem.obj();
                add_to_change_list(b.name.translated(),b.description.translated());
            }
            add_to_change_list(" ");
        }
    }
    else {
        
        bool flag = true;
        if (!currpage->bionics.empty()) {
            for (const auto elem : currpage->bionics) {
                
                if (std::find(prevpage->bionics.begin(), prevpage->bionics.end(), elem) == prevpage->bionics.end()) {
                    if (flag) {
                        add_to_change_list(_("New Bionics: "));
                        flag = false;
                    }
                    const bionic_data& b = elem.obj();
                    add_to_change_list(b.name.translated(),b.description.translated());
                }
            }
            if(!flag) add_to_change_list(" ");
        }
       
        flag = true;
        if (!prevpage->bionics.empty()) {
            for (const auto elem : prevpage->bionics) {
                if (std::find(currpage->bionics.begin(), currpage->bionics.end(), elem) == currpage->bionics.end()) {
                    if (flag) {
                        add_to_change_list(_("Lost Bionics: "));
                        flag = false;
                    }
                    const bionic_data& b = elem.obj();                    
                    add_to_change_list(b.name.translated(), b.description.translated());
                }
            }
            if (!flag) add_to_change_list(" ");
        }
        
    }
}

void diary::kill_changes() {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        if (!currpage->kills.empty()) {
            add_to_change_list( _("Kills: "));
            for (const auto elem : currpage->kills) {
                const mtype& m = elem.first.obj();
                nc_color color = m.color;
                std::string symbol = m.sym;
                std::string nname = m.nname(elem.second);
                add_to_change_list(string_format("%4d ", elem.second) + colorize(symbol,
                    color) + " " + colorize(nname, c_light_gray),m.get_description());                    
            }
            add_to_change_list(" ");
        }
        if (!currpage->npc_kills.empty()) {
            add_to_change_list(_("NPC Killed"));
            for (const auto& npc_name : currpage->npc_kills) {
                add_to_change_list(string_format("%4d ", 1) + colorize("@ " + npc_name, c_magenta));
            }
            add_to_change_list(" ");
        }
        
    }
    else {
        
        if (!currpage->kills.empty()) {
            
            bool flag = true;
            for (const auto elem : currpage->kills) {
                const mtype& m = elem.first.obj();
                nc_color color = m.color;
                std::string symbol = m.sym;
                std::string nname = m.nname(elem.second);
                int kills = elem.second;
                if (prevpage->kills.count(elem.first) > 0) {
                    const int prevkills = prevpage->kills[elem.first];
                    if (kills > prevkills) {
                        if (flag) {
                            add_to_change_list(_("Kills: "));
                            flag = false;
                        }
                        kills = kills - prevkills;
                        add_to_change_list(string_format("%4d ", kills) + colorize(symbol,
                            color) + " " + colorize(nname, c_light_gray),m.get_description());
                    }
                }
                else {
                    if (flag) {
                        add_to_change_list(_("Kills: "));
                        flag = false;
                    }
                    add_to_change_list(string_format("%4d ", kills) + colorize(symbol,
                        color) + " " + colorize(nname, c_light_gray), m.get_description());
                }
                
                
            }
            if (!flag) add_to_change_list(" ");
           
        }
        if (!currpage->npc_kills.empty()) {
            
            const auto prev_npc_kills = prevpage->npc_kills;
            
            bool flag = true;
                for (const auto& npc_name : currpage->npc_kills) {
                    
                        if ((std::find(prev_npc_kills.begin(), prev_npc_kills.end(), npc_name) == prev_npc_kills.end())) {
                            if (flag) {
                                add_to_change_list(_("NPC Killed: "));
                                flag = false;
                            }
                            add_to_change_list(string_format("%4d ", 1) + colorize("@ " + npc_name, c_magenta));
                        }
                    
                }
                if (!flag) add_to_change_list(" ");
           
        }        
        
        
    }
}


void diary::skill_changes() {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        if (currpage->skillsL.empty()) {
            return;
        }
        else {
            
            add_to_change_list(_("Skills:"));
            for (auto elem : currpage->skillsL) {
                
                if (elem.second.level() > 0) {
                    Skill s = elem.first.obj();
                    add_to_change_list(string_format("<color_light_blue>%s: %d</color>", s.name(), elem.second.level()), s.description());
                }
            }
            add_to_change_list("");
        }
    }
    else
    {
        
        bool flag = true;
        for (auto elem : currpage->skillsL) {
            if (prevpage->skillsL.count(elem.first) > 0) {
                if (prevpage->skillsL[elem.first].level() != elem.second.level()) {
                    if (flag) {
                        add_to_change_list(_("Skills: "));
                        flag = false;
                    }
                    Skill s = elem.first.obj();
                    add_to_change_list(string_format(_("<color_light_blue>%s: %d -> %d</color>"), s.name(), prevpage->skillsL[elem.first].level(), elem.second.level()),s.description());
                }

            }
            
        }
        if (!flag) add_to_change_list(" ");
        
    }
}

void diary::trait_changes() {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        if (!currpage->traits.empty()) {
            add_to_change_list(_("Mutations:"));
            for (const trait_id elem : currpage->traits) {
                const mutation_branch& trait = elem.obj();               
                add_to_change_list(colorize(trait.name(), trait.get_display_color()),trait.desc());
            }
            add_to_change_list("");
        }
    }
    else {
        if (prevpage->traits.empty() && !currpage->traits.empty()) {
            add_to_change_list(_("Mutations:"));
            for (const trait_id& elem : currpage->traits) {
                const mutation_branch& trait = elem.obj();
                add_to_change_list(colorize(trait.name(), trait.get_display_color()), trait.desc());
                
            }
            add_to_change_list("");
        }
        else {
            
            bool flag = true;
            for (const trait_id& elem : currpage->traits) {
                
                if(std::find(prevpage->traits.begin(), prevpage->traits.end(),elem) == prevpage->traits.end()){
                    if (flag) {
                        add_to_change_list(_("Gained Mutation: "));
                        flag = false;
                    }
                    const mutation_branch& trait = elem.obj();
                    add_to_change_list(colorize(trait.name(), trait.get_display_color()), trait.desc());
                }
                
            }
            if (!flag) add_to_change_list(" ");
            
            flag = true;
            for (const trait_id& elem : prevpage->traits) {
                
                if (std::find(currpage->traits.begin(), currpage->traits.end(), elem) == currpage->traits.end()) {
                    if (flag) {
                        add_to_change_list(_("Lost Mutation "));
                        flag = false;
                    }
                    const mutation_branch& trait = elem.obj();
                    add_to_change_list(colorize(trait.name(), trait.get_display_color()), trait.desc());
                }
            }
            if (!flag) add_to_change_list(" ");
            
        }
    }
}

void diary::stat_changes() {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        add_to_change_list(_("Stats:"));
        add_to_change_list(string_format(_("Strength: %d"), currpage->strength));
        add_to_change_list(string_format(_("Dexterity: %d"), currpage->dexterity));
        add_to_change_list(string_format(_("Intelligence: %d"), currpage->intelligence));
        add_to_change_list(string_format(_("Perception: %d"), currpage->perception));
        add_to_change_list(" ");
    }
    else {
        
        bool flag = true;        
        if (currpage->strength != prevpage->strength) {
            if (flag) {
                add_to_change_list(_("Stats: "));
                flag = false;
            }
            add_to_change_list(string_format(_("Strength: %d -> %d"), prevpage->strength, currpage->strength));
        }
        if (currpage->dexterity != prevpage->dexterity) { 
            if (flag) {
                add_to_change_list(_("Stats: "));
                flag = false;
            }
            add_to_change_list(string_format(_("Desxterity: %d -> %d"), prevpage->dexterity, currpage->dexterity)); 
        }
        if (currpage->intelligence != prevpage->intelligence) {
            if (flag) {
                add_to_change_list(_("Stats: "));
                flag = false;
            }
            add_to_change_list(string_format(_("Intelligence: %d -> %d"), prevpage->intelligence, currpage->intelligence));
        }

        if (currpage->perception != prevpage->perception) { 
            if (flag) {
                add_to_change_list(_("Stats: "));
                flag = false;
            }
            add_to_change_list(string_format(_("Perception: %d -> %d"), prevpage->perception, currpage->perception)); 
        }
        if (!flag)add_to_change_list(" ");
        
    }
}

void diary::prof_changes() {
    diary_page* currpage = get_page_ptr();
    diary_page* prevpage = get_page_ptr(-1);
    if (currpage == nullptr)return;
    if (prevpage == nullptr) {
        if (!currpage->known_profs.empty()) {
            add_to_change_list(_("Proficiencies:"));
            for (const proficiency_id elem : currpage->known_profs) {
                const auto p = elem.obj();                
                add_to_change_list(p.name(),p.description());
            }
            add_to_change_list("");
        }
        
    }
    else {
        
        bool flag = true;
        for (const proficiency_id elem : currpage->known_profs) {
            
            if (std::find(prevpage->known_profs.begin(), prevpage->known_profs.end(), elem) == prevpage->known_profs.end()) {
                if (flag) {
                    add_to_change_list(_("Proficiencies: "));
                    flag = false;
                }
                const auto p = elem.obj();
                add_to_change_list(p.name(), p.description());
            }
            
        }
        if (!flag) add_to_change_list(" ");
        
        
    }
}

std::vector<std::string> diary::get_change_list() {
    if (!change_list.empty()) return change_list;
    if (!pages.empty()) {
        
        

        stat_changes();
        skill_changes();
        prof_changes();
        trait_changes();
        bionic_changes();
        spell_changes();
        mission_changes();
        kill_changes();
        
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
    page -> mission_completed = mission::to_uid_vector(u->get_completed_missions());
    page -> mission_active = mission::to_uid_vector(u->get_active_missions());
    page -> mission_faild = mission::to_uid_vector(u->get_failed_missions());
    
    
    
    
    page -> male = u->male;
    page->strength = u->get_str_base();
    page->dexterity = u->get_dex_base();
    page->intelligence = u->get_int_base();
    page->perception = u->get_per_base();
    //page->addictions = u->addictions;
    
    page -> follower_ids = u->follower_ids; 
    
    page->traits = u->get_mutations(false);
    
    //page->magic = u->magic ;
    const auto spells = u->magic->get_spells();
    for (const auto spell : spells) {
        const auto id = spell->id();
        const auto lvl = spell->get_level();
        
        page-> known_spells[id] = lvl;
        
        /*auto spell = u->magic->get_spell(id);
        spell.name();
        spell.description();*/
    }
    
   
    //page -> martial_arts_data = u->martial_arts_data;
    
    page-> bionics = u->get_bionics();
    
    
    

    for (auto elem : Skill::skills) {
        
        SkillLevel level = u->get_skill_level_object(elem.ident());
        page->skillsL.insert({ elem.ident(), level });
    }
    
    page -> known_profs = u->_proficiencies->known_profs();
    page -> learning_profs = u->_proficiencies->learning_profs();
    
    
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
        jout.member("completed", n->mission_completed);
        jout.member("active", n->mission_active);
        jout.member("faild", n->mission_faild);
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
        jout.member("spells", n->known_spells);
        jout.member("skillsL", n->skillsL);
                   
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
        
        for (JsonObject elem : data.get_array("pages")) {
            diary_page* page = new diary_page();
            page->m_date = elem.get_string("date");
            page->m_text = elem.get_string("text");
            elem.read("turn", page->turn);
            elem.read("active", page->mission_active);
            elem.read("completed", page->mission_completed);
            elem.read("faild", page->mission_faild);
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
            elem.read("spells", page->known_spells);
            elem.read("skillsL", page->skillsL);
            
            
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


