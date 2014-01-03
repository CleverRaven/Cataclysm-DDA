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

enum mod_type {
    MT_CORE,
    MT_SUPPLEMENTAL
};
struct MOD_INFORMATION {
    std::string path;
    std::string name;
    std::string ident;
    std::string author;
    std::string description;
    mod_type _type;
    std::vector<std::string> dependencies;

    MOD_INFORMATION()
        : _type(MT_SUPPLEMENTAL) {
    }

    std::string type() {
        switch (_type) {
            case MT_CORE:
                return "CORE";
                break;
            case MT_SUPPLEMENTAL:
                return "SUPPLEMENTAL";
                break;
        }
    }
};

class mod_manager
{
    public:
        mod_manager();
        virtual ~mod_manager();
        /**
         * Reload the map of available mods (@ref mod_map).
         * This also reloads the dependency tree.
         */
        void refresh_mod_list();

        void show_ui();
        /**
         * Returns the dependency tree for the loaded mods.
         * @returns @ref dependency_tree
         */
        dependency_tree &get_tree();
        /**
         * Clear @ref mod_map and delete @ref dependency_tree.
         */
        void clear();
        /**
         * @returns true if the mod manager knows a mod with this ident.
         */
        bool has_mod(const std::string &ident) const;
        /**
         * Copy the json files of the listed mods into the
         * given folder (output_base_path)
         * @param mods_to_copy A list (of idents) of mods whose
         * json data should be copied.
         * @param output_base_path The folder where to put the copies.
         * The function creates a sub folder for each mod and
         * puts the files there. The original folder structure is
         * retained.
         */
        bool copy_mod_contents(std::vector<std::string> mods_to_copy, std::string output_base_path);
    protected:
    private:
        // Make this accessible for now
        friend class mod_ui;
        friend class worldfactory;
        friend class game;
        /**
         * Load all modinfo.json files (recursively) from the
         * given root.
         * @param path The root folder from which the modinfo
         * files are searched.
         */
        void load_mods_from(std::string path);
        /**
         * Load all mod information from a json file.
         * (@see load_modfile)
         */
        void load_mod_info(std::string info_file_path);
        /**
         * Load mod info from a json object. Put the loaded modinfo
         * directly into @ref mod_map.
         * @throws std::string on all kind of errors. The string
         * contains the error message.
         */
        void load_modfile(JsonObject &jo, const std::string &main_path);

        dependency_tree tree;

        typedef std::map<std::string, MOD_INFORMATION *> t_mod_map;
        /**
         * The map of known mod, key is the mod ident. Values are
         * never NULL.
         */
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

        void try_add(int selection, std::vector<std::string> modlist,
                     std::vector<std::string> &active_list);
        void try_rem(int selection, std::vector<std::string> &active_list);
        void try_shift(char direction, int &selection, std::vector<std::string> &active_list);

        bool can_shift_up(int selection, std::vector<std::string> active_list);
        bool can_shift_down(int selection, std::vector<std::string> active_list);
    private:

        void show_mod_information(WINDOW *win, int width, MOD_INFORMATION *mod, std::string note);
        void draw_layering_ui_lines(WINDOW *win);
        void draw_headers(WINDOW *win, int sy, const std::vector<std::string> headers,
                          unsigned char selected_header);
        void draw_modlist(WINDOW *win, int sy, int sx, const std::vector<std::string> modlist,
                          bool header_active, int last_selection);
        void draw_shift_information(WINDOW *win, int sy, int sx,
                                    const std::vector<std::string> active_mods_list, const int selection);
        int gather_input(int &active_header, int &selection, std::vector<std::string> mod_list,
                         std::vector<std::string> &active_mods_list);


        void set_usable_mods();
};

#endif // MOD_MANAGER_H
