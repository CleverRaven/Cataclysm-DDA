#pragma once
#ifndef CATA_SRC_DIARY_H
#define CATA_SRC_DIARY_H


#include <string>
#include <list>
#include <vector>
#include <ui.h>

    

#include "kill_tracker.h"
#include "stats_tracker.h"
#include "achievement.h"
#include "character.h"
#include "units.h"
#include "skill.h"

//namespace diary_strukt {
    struct diary_page
    {

        diary_page(std::string date, std::string text);
        diary_page();

        //void fill_with_gamestate();
        std::string m_text;
        std::string m_date;

        /*
        * turn
        * kill_tracker
        * stats_tracker
        * achievements_tracker
        * avatar
            * active_mission
            * completet_mission
            * failed_mission
            * player
                * male
                * addictions
                * followers
                * charakter
                    * str_max .. und die anderen
                    * str_bonus
                    * traits = unordert set
                    * mutations =unordert map
                    * magic = pimpl<T>
                    * martial_arts_data = pimpl<T>
                    * my_bionics = pimpl<T>
                    * skills = pimpl<skill_level_map
                    * proficencies = pimpl<T>
                    * max_power_level = energy

        */
        time_point privius_Page_turn;
        std::vector<std::string> diff_to_previus_page;

        time_point turn;
        ////game stats

        std::map<mtype_id, int> kills;
        std::vector<std::string> npc_kills;
        //pimpl<stats_tracker> stats_tracker;
        //pimpl<achievements_tracker> achivement_tracker;
        ////player Stats
        bool male;
        int strength;
        int dexterity;
        int intelligence;
        int perception;
        //std::vector<addiction> addictions;
        std::set<character_id> follower_ids;
        //std::unordered_set<trait_id> traits;
        std::vector<trait_id> traits;
        //std::unordered_map<trait_id, Character::trait_data> mutations;
        //pimpl<known_magic> magic;
        //pimpl<character_martial_arts> martial_arts_data;
        std::vector<bionic_id> bionics;

        //pimpl<SkillLevelMap> skills;
        //std::vector<Skill> skills;
        std::map<skill_id, SkillLevel> skillsL;
        //std::vector<std::string> skill;
        std::vector<proficiency_id> known_profs;
        std::vector<proficiency_id> learning_profs;
        units::energy max_power_level;
    };
//}
class diary 
{
    
    
    //attribute
    public:
        std::string owner;
        std::vector<diary_page *> pages;
    private:
        int opend_page = 0;
        std::vector<std::string> change_list;
        std::map<int,std::string> desc_map;
        

    //methoden
    public:
        diary();
        void load_test();
        int set_opend_page(int i);
        void edit_page_ui();
        void new_page();
        void delete_page();

        int get_opend_page_num();
        std::vector<std::string> get_pages_list();
        std::vector<std::string> get_change_list();
        std::map<int, std::string> get_desc_map();

        diary_page* get_page_ptr(int offset = 0);
        std::string get_page_text();
        std::string get_head_text();

        void skill_changes();
        void kill_changes();
        void trait_changes();
        void bionic_changes();
        void stat_changes();
        void prof_changes();

        void set_page_text( std::string text);
        static void show_diary_ui(diary* c_diary);

        

        void export_to_txt();
        void serialize();
        void deserialize();
        void serialize(std::ostream& fout);
        void deserialize(std::istream& fin);
    private:
        void add_to_change_list(std::string entry, std::string desc = "");

};
 



#endif //CATA_SCR_DIARY_H
