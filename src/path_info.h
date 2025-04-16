#pragma once
#ifndef CATA_SRC_PATH_INFO_H
#define CATA_SRC_PATH_INFO_H

#include "cata_path.h"

#include <string>

enum class holiday : int;

const std::string SAVE_MASTER( "master.gsav" );
const std::string SAVE_ARTIFACTS( "artifacts.gsav" );
const std::string SAVE_EXTENSION( ".sav" );
const std::string SAVE_EXTENSION_LOG( ".log" );
const std::string SAVE_EXTENSION_WEATHER( ".weather" );
const std::string SAVE_EXTENSION_SHORTCUTS( ".shortcuts" );

namespace PATH_INFO
{

void init_base_path( const std::string &path );
void init_user_dir( std::string dir );
void set_standard_filenames();

std::string cache_dir();
std::string config_dir();
std::string datadir();
std::string defaulttilejson();
std::string defaultlayeringjson();
std::string defaulttilepng();
std::string fontdir();
std::string user_font();
std::string keymap();
std::string memorialdir();
std::string achievementdir();
std::string savedir();
std::string sokoban();
std::string templatedir();
std::string user_dir();
std::string user_moddir();
std::string worldoptions();
std::string world_timestamp();
std::string crash();
std::string tileset_conf();
std::string langdir();
std::string lang_file();
std::string soundpack_conf();

std::string credits();
std::string motd();
std::string title( holiday current_holiday );

cata_path autopickup();
cata_path autonote();
cata_path base_colors();
cata_path base_path();
cata_path color_templates();
cata_path color_themes();
cata_path colors();
cata_path config_dir_path();
cata_path custom_colors();
cata_path data_sound();
cata_path datadir_path();
cata_path debug();
cata_path defaultsounddir();
cata_path fontdata();
cata_path gfxdir();
cata_path graveyarddir_path();
cata_path jsondir();
cata_path keybindings();
cata_path keybindings_vehicle();
cata_path langdir_path();
cata_path lastworld();
cata_path legacy_fontdata();
cata_path memorialdir_path();
cata_path achievementdir_path();
cata_path moddir();
cata_path mods_dev_default();
cata_path mods_user_default();
cata_path mods_replacements();
cata_path names();
cata_path options();
cata_path panel_options();
cata_path player_base_save_path();
cata_path pocket_presets();
cata_path safemode();
cata_path savedir_path();
cata_path templatedir_path();
cata_path user_dir_path();
cata_path user_gfx();
cata_path user_keybindings();
cata_path user_moddir_path();
cata_path user_sound();
cata_path world_base_save_path();

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
