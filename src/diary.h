#pragma once
#ifndef CATA_SRC_DIARY_H
#define CATA_SRC_DIARY_H

#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "calendar.h"
#include "mutation.h"
#include "type_id.h"
#include "units.h"

class JsonOut;
class JsonValue;

namespace catacurses
{
class window;
}  // namespace catacurses

/// <summary>
/// diary page, to save current character progression
/// </summary>
struct diary_page {
    diary_page();
    virtual ~diary_page() = default;
    /*the text the player added to the page*/
    std::string m_text;
    time_point previous_Page_turn;
    std::vector<std::string> diff_to_previous_page;
    /*turn the page was created*/
    time_point turn;
    /*accuracy of time recorded in journal entry:
    2 = player has a watch; 1 = player is able to see the sky; 0 = no idea what time is it now*/
    time_accuracy time_acc = time_accuracy::NONE;
    /*mission ids for completed/active and failed missions*/
    std::vector<int> mission_completed;
    std::vector<int> mission_active;
    std::vector<int> mission_failed;
    /*monster id and kill count of Killed monster*/
    std::map<mtype_id, int> kills;
    /*names of killed npc`s*/
    std::vector<std::string> npc_kills;
    /*gender*/
    bool male = false;
    /*base character stats*/
    int strength = 0;
    int dexterity = 0;
    int intelligence = 0;
    int perception = 0;
    /*traits id the character has - as well as their variant ids (empty if none)*/
    std::vector<trait_and_var> traits;
    /*spells id with level the character has*/
    std::map<spell_id, int> known_spells;
    /*bionics id`s the character has*/
    std::vector<bionic_id> bionics;
    /*skill id's with level the character has*/
    std::map<skill_id, int> skillsL;
    /*known and learning profession id of the character*/
    std::vector<proficiency_id> known_profs;
    std::vector<proficiency_id> learning_profs;
    /*maximal power level the character has*/
    units::energy max_power_level;
};

/// <summary>
/// Diary is connected to the player avatar.
/// The player is able to add new pages.
/// Every page saves the current character progression and shows the improvements compared to the previous pages.
/// The player is also able to add a Text in every page.
/// </summary>
class diary
{
        //attribute
    private:
        /*character name who owns the diary*/
        std::string owner;
        /*list of all pages added to the diary*/
        std::vector< std::unique_ptr<diary_page>> pages;

        /*current opened page*/
        int opened_page = 0; // NOLINT(cata-serialize)
        /*list of changes from opened page to previous page*/
        std::vector<std::string> change_list; // NOLINT(cata-serialize)
        /*maps description to position in change list*/
        std::map<int, std::string> desc_map; // NOLINT(cata-serialize)

        //methods
    public:
        diary();
        virtual ~diary() = default;
        /*static method to open a diary ui*/
        static void show_diary_ui( diary *c_diary );
        /*last entry in the diary, will be called after character death */
        void death_entry();

        /*serialize and deserialize*/
        bool store();
        void load();
        void serialize( std::ostream &fout );
        void deserialize( const JsonValue &jsin );
        void serialize( JsonOut &jsout );

    private:
        /*Uses editor window class to edit the text.*/
        void edit_page_ui( const std::function<catacurses::window()> &create_window );
        /*set page to be be shown in ui*/
        int set_opened_page( int pagenum );
        /*create a new page and adds current character progression*/
        void new_page();
        /*set page text*/
        void set_page_text( std::string text );
        /*delite current page*/
        void delete_page();

        /*get opened page nummer*/
        int get_opened_page_num() const;
        /*returns a list with all pages by the its date*/
        std::vector<std::string> get_pages_list();
        /*returns a list with all changes compared to the previous page*/
        std::vector<std::string> get_change_list();
        /*returns a map corresponding to the change_list with descriptions*/
        std::map<int, std::string> get_desc_map();

        /*returns pointer to current page*/
        diary_page *get_page_ptr( int offset = 0 );
        /*returns the text of opened page*/
        std::string get_page_text();
        /*returns text for head of page*/
        std::string get_head_text();
        /*the following methods are used to fill the change_list and desc_map in comparison to the previous page*/
        void skill_changes();
        void kill_changes();
        void trait_changes();
        void bionic_changes();
        void stat_changes();
        void prof_changes();
        void mission_changes();
        void spell_changes();

        /*expots the diary to a readable .txt file. If its the lastexport, its exportet to memorial otherwise its exportet to the world folder*/
        void export_to_txt( bool lastexport = false );
        /*method for adding changes to the changelist. with the possibility to connect a description*/
        void add_to_change_list( const std::string &entry, const std::string &desc = "" );
};
#endif // CATA_SRC_DIARY_H
