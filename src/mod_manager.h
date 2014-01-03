#ifndef MOD_MANAGER_H
#define MOD_MANAGER_H

#include "dependency_tree.h"
#include "json.h"

#include "cursesdef.h"
#include <string>
#include <vector>
#include <map>

class mod_ui;
class game;
class worldfactory;

enum mod_type
{
    MT_CORE,
    MT_SUPPLEMENTAL
};
struct MOD_INFORMATION
{
    std::string path, name, ident, author, description;
    mod_type _type;
    std::vector<std::string> dependencies;

    MOD_INFORMATION()
    : _type(MT_SUPPLEMENTAL)
    {
    }

    std::string type()
    {
        switch (_type){
            case MT_CORE: return "CORE"; break;
            case MT_SUPPLEMENTAL: return "SUPPLEMENTAL"; break;
        }
    }
};

class mod_manager
{
    public:
        mod_manager();
        virtual ~mod_manager();

        void refresh_mod_list();

        void show_ui();

        dependency_tree *get_tree();
        void clear();
        /**
         * @returns true if the mod anager knows a mod with this ident.
         */
        bool has_mod(const std::string &ident) const;

        bool copy_mod_contents(std::vector<std::string> mods_to_copy, std::string output_base_path);
    protected:
    private:
        // Make this accessible for now
        friend class mod_ui;
        friend class worldfactory;
        friend class game;
        bool load_mods_from(std::string path);
        bool load_mod_info(std::string info_file_path);
        void load_modfile(JsonObject &jo, const std::string &main_path);

        dependency_tree *tree;

        typedef std::map<std::string, MOD_INFORMATION *> t_mod_map;
        t_mod_map mod_map;
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
