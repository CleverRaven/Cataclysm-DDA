#pragma once
#ifndef CATA_SRC_PATH_INFO_H
#define CATA_SRC_PATH_INFO_H

#include <string>

enum class holiday : int;

namespace PATH_INFO
{
void init_base_path( std::string path );
void init_user_dir( std::string dir );
void set_standard_filenames();

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
std::string user_font();
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
std::string user_keybindings();
std::string user_moddir();
std::string worldoptions();
std::string crash();
std::string tileset_conf();
std::string gfxdir();
std::string langdir();
std::string lang_file();
std::string user_gfx();
std::string data_sound();
std::string user_sound();
std::string mods_replacements();
std::string mods_dev_default();
std::string mods_user_default();
std::string soundpack_conf();

std::string credits();
std::string motd();
std::string title( holiday current_holiday );
std::string names();

void set_datadir( const std::string &datadir );
void set_config_dir( const std::string &config_dir );
void set_savedir( const std::string &savedir );
void set_memorialdir( const std::string &memorialdir );
void set_options( const std::string &options );
void set_keymap( const std::string &keymap );
void set_autopickup( const std::string &autopickup );
void set_motd( const std::string &motd );

} // namespace PATH_INFO

#endif // CATA_SRC_PATH_INFO_H
