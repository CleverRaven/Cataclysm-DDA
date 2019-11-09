#pragma once
#ifndef PATH_INFO_H
#define PATH_INFO_H

#include <string>

namespace PATH_INFO
{
void init_base_path( std::string path );
void init_user_dir( const char *ud = "" );
void update_datadir();
void update_config_dir();
void update_pathname( const std::string &name, const std::string &path );
void set_standard_filenames();
/**
 * Return a locale specific path, or if there is no path for the current
 * locale, return the default path.
 * @param pathid The key in the @ref FILENAMES map. The local path is based
 * on that value.
 * @param extension File name extension, is automatically added to the path
 * of the translated file. Can be empty, but must otherwise include the
 * initial '.', e.g. ".json"
 * @param fallbackid The path id of the fallback filename. As like pathid it's
 * the key into the @ref FILENAMES map. It is used if no translated file can be
 * found.
 */
std::string find_translated_file( const std::string &pathid, const std::string &extension,
                                  const std::string &fallbackid );
std::string autopickup();
std::string base_colors();
std::string base_path();
std::string colors();
std::string color_templates();
std::string config_dir();
std::string custom_colors();
std::string datadir();
std::string debug();
std::string defaultsounddir();
std::string defaulttilejson();
std::string defaulttilepng();
std::string fontdata();
std::string fontdir();
std::string fontlist();
std::string graveyarddir();
std::string help();
std::string keybindings();
std::string keybindings_vehicle();
std::string keymap();
std::string lastworld();
std::string legacy_autopickup();
std::string legacy_autopickup2();
std::string legacy_fontdata();
std::string legacy_keymap();
std::string legacy_options();
std::string legacy_options2();
std::string legacy_worldoptions();
std::string memorialdir();
std::string jsondir();
std::string moddir();
std::string options();
std::string panel_options();
std::string safemode();
std::string savedir();
std::string sokoban();
std::string templatedir();
std::string user_dir();
std::string user_gfx();
std::string user_keybindings();
std::string user_moddir();
std::string user_sound();
std::string worldoptions();
std::string crash();
std::string tileset_conf();
std::string gfxdir();
std::string user_gfx();
std::string data_sound();
std::string user_sound();
std::string mods_replacements();
std::string mods_dev_default();
std::string mods_user_default();
std::string soundpack_conf();

} // namespace PATH_INFO

#endif
