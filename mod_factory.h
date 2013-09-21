#ifndef MOD_FACTORY_H
#define MOD_FACTORY_H

#include "catajson.h"
#include "catacurse.h"
#include "debug.h"

#include <vector>
#include <string>

enum mod_type
{
    MOD_UNKNOWN,
    MOD_CORE,
    MOD_SUPPLEMENTAL
};
struct MOD_INFORMATION
{
    std::string _path;
    std::string _author;
    mod_type _type;     // MOD_CORE or MOD_SUPPLEMENTAL, determines reordering ability
    std::string _ident; // identification name for this mod
    std::string _name;  // visible name for the mod
    std::string _desc;  // short description of the mod

    std::vector<std::string> _dependencies; // list of dependencies this mod has, if any, on other supplementary mods

    std::string author() {return _author;}
    std::string path(){ return _path;}
    std::string type()
    {
        switch(_type)
        {
            case MOD_CORE: return "CORE"; break;
            case MOD_SUPPLEMENTAL: return "SUPPLEMENTAL"; break;
            default: return "UNKNOWN TYPE"; break;
        }
    }
    mod_type raw_type()
    {
        return _type;
    }
    std::string ident(){ return _ident;}
    std::string name() { return _name;}
    std::string desc() { return _desc;}
    std::vector<std::string> dependencies() { return _dependencies;}
};
/*
    The Mod Factory class deals with loading and ordering mod packs, and preparing them for inclusion into the primary game
*/
class mod_factory
{
    public:
        /** Default constructor */
        mod_factory();
        /** Default destructor */
        virtual ~mod_factory();

        void refresh_mod_list();
        void show_mod_layering_ui();
        bool consolidate_mod_layers(std::vector<std::string > layers);

        std::vector<MOD_INFORMATION*> mods;
    protected:
    private:
        bool load_mods_from(std::string path);
        bool load_mod_info(MOD_INFORMATION *mod);

        void show_mod_information(WINDOW* win, int sx, int sy, int width, MOD_INFORMATION *mod, std::string note);
        void draw_layering_ui_lines(WINDOW *win);
        void try_add(int selection, std::vector<std::string> modlist, std::vector<std::string> &activelist, std::map<std::string, MOD_INFORMATION*> modmap);
        void try_rem(int selection, std::vector<std::string> &activelist, std::map<std::string, MOD_INFORMATION*> modmap);
        void try_shift(char direction, int &selection, std::vector<std::string > &activelist, std::map<std::string, MOD_INFORMATION* > modmap);
        std::vector<std::string> get_mods_dependent_on(std::string dependency, std::vector<std::string> activelist, std::map<std::string, MOD_INFORMATION*> modmap);

        bool can_shift_up(int selection, std::vector<std::string > activelist, std::map<std::string, MOD_INFORMATION* > modmap);
        bool can_shift_down(int selection, std::vector<std::string > activelist, std::map<std::string, MOD_INFORMATION* > modmap);

        void print_debug_mods();
};

#endif // MOD_FACTORY_H
