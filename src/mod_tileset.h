#pragma once
#ifndef CATA_SRC_MOD_TILESET_H
#define CATA_SRC_MOD_TILESET_H

#include <string> // IWYU pragma: keep
#include <string_view>
#include <vector>

#include "cata_path.h"

class JsonObject;
class mod_tileset;

extern std::vector<mod_tileset> all_mod_tilesets;

void load_mod_tileset( const JsonObject &jsobj, std::string_view, const cata_path &base_path,
                       const cata_path &full_path );
void reset_mod_tileset();

class mod_tileset
{
    public:
        mod_tileset( const cata_path &new_base_path, const cata_path &new_full_path,
                     int new_num_in_file ) :
            base_path_( new_base_path ),
            full_path_( new_full_path ),
            num_in_file_( new_num_in_file ) { }

        const cata_path &get_base_path() const {
            return base_path_;
        }

        const cata_path &get_full_path() const {
            return full_path_;
        }

        int num_in_file() const {
            return num_in_file_;
        }

        bool is_compatible( const std::string &tileset_id ) const;
        void add_compatible_tileset( const std::string &tileset_id );

    private:
        cata_path base_path_;
        cata_path full_path_;
        int num_in_file_;
        std::vector<std::string> compatibility;
};

#endif // CATA_SRC_MOD_TILESET_H
