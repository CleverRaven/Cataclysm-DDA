#ifndef MOD_MANAGER_H
#define MOD_MANAGER_H

#include "dependency_tree.h"
#include "json.h"

#include "cursesdef.h"
#include <string>
#include <vector>
#include <map>

enum mod_type
{
    MT_UNKNOWN,
    MT_CORE,
    MT_SUPPLEMENTAL
};
struct MOD_INFORMATION
{
    std::string path, name, ident, author, description;
    mod_type _type;
    std::vector<std::string> dependencies;

    std::string type()
    {
        switch (_type){
            case MT_CORE: return "CORE"; break;
            case MT_SUPPLEMENTAL: return "SUPPLEMENTAL"; break;
            default: return "UNKNOWN TYPE"; break;
        }
    }
};

class mod_manager
{
    public:
        /** Default constructor */
        mod_manager();
        /** Default destructor */
        virtual ~mod_manager();

        void refresh_mod_list();

        std::vector<MOD_INFORMATION*> mods;
        std::map<std::string, unsigned> mod_map;

        MOD_INFORMATION *load_modfile(JsonObject &jo);
        void show_ui();

        dependency_tree *get_tree(){ return tree;}
        void clear();

        bool copy_mod_contents(std::vector<std::string> mods_to_copy, std::string output_base_path);
    protected:
    private:
        bool load_mods_from(std::string path);
        bool load_mod_info(MOD_INFORMATION *mod, std::string info_file_path);

        dependency_tree *tree;
};

class mod_ui
{
public:
    mod_ui(mod_manager *modman);
    virtual ~mod_ui();

    int show_layering_ui();
    std::vector<std::string> usable_mods;
    std::string get_information(MOD_INFORMATION *mod);
    mod_manager *active_manager;
    dependency_tree *mm_tree;

    void try_add(int selection, std::vector<std::string> modlist, std::vector<std::string> &active_list);
    void try_rem(int selection, std::vector<std::string> &active_list);
    void try_shift(char direction, int &selection, std::vector<std::string> &active_list);

    bool can_shift_up(int selection, std::vector<std::string> active_list);
    bool can_shift_down(int selection, std::vector<std::string> active_list);
private:

    void show_mod_information(WINDOW *win, int width, MOD_INFORMATION *mod, std::string note);
    void draw_layering_ui_lines(WINDOW *win);
    void draw_headers(WINDOW *win, int sy, const std::vector<std::string> headers, unsigned char selected_header);
    void draw_modlist(WINDOW *win, int sy, int sx, const std::vector<std::string> modlist, bool header_active, int last_selection);
    void draw_shift_information(WINDOW *win, int sy, int sx, const std::vector<std::string> active_mods_list, const int selection);
    int gather_input(int &active_header, int &selection, std::vector<std::string> mod_list, std::vector<std::string> &active_mods_list);


    void set_usable_mods();
};

#endif // MOD_MANAGER_H
