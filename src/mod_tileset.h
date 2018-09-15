#pragma once
#ifndef MOD_TILESET_H
#define MOD_TILESET_H

#include <string>
#include <vector>

class mod_tileset;
class JsonObject;

extern std::vector<mod_tileset *> all_mod_tilesets;

void load_mod_tileset( JsonObject &jsobj, const std::string &, const std::string &base_path,
                       const std::string &full_path );
void load_mod_tileset_into_array( JsonObject &jsobj, const std::string &base_path,
                                  const std::string &full_path,
                                  std::vector<mod_tileset *> &mod_tileset_array );
void reset_mod_tileset();

class mod_tileset
{
    public:
        mod_tileset() { }

        mod_tileset( const std::string &new_base_path, const std::string &new_full_path ) :
            base_path_( new_base_path ),
            full_path_( new_full_path ) { }

        mod_tileset( const mod_tileset &obj ) {
            base_path_ = obj.base_path_;
            full_path_ = obj.full_path_;
        }
        ~mod_tileset() {
            compatibility.clear();
        }

        const std::string &get_base_path() const {
            return base_path_;
        }

        const std::string &get_full_path() const {
            return full_path_;
        }

        bool is_compatible( std::string tileset_id ) const;
        void add_compatible_tileset( std::string tileset_id );

    private:
        std::string base_path_;
        std::string full_path_;
        std::vector<std::string> compatibility;
};

#endif
