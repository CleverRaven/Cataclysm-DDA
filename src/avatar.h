#pragma once
#ifndef AVATAR_H
#define AVATAR_H

#include "player.h"

class avatar : public player
{
    public:
        avatar();

        void store( JsonOut &json ) const;
        void load( JsonObject &data );
        void serialize( JsonOut &josn ) const;
        void deserialize( JsonIn &json );
        void serialize_map_memory( JsonOut &jsout ) const;
        void deserialize_map_memory( JsonIn &jsin );

        // newcharacter.cpp
        bool create( character_type type, const std::string &tempname = "" );
        void randomize( bool random_scenario, points_left &points, bool play_now = false );
        bool load_template( const std::string &template_name, points_left &points );

        /** Prints out the player's memorial file */
        void memorial( std::ostream &memorial_file, const std::string &epitaph );

        void toggle_map_memory();
        bool should_show_map_memory();
        /** Memorizes a given tile in tiles mode; finalize_tile_memory needs to be called after it */
        void memorize_tile( const tripoint &pos, const std::string &ter, const int subtile,
                            const int rotation );
        /** Returns last stored map tile in given location in tiles mode */
        memorized_terrain_tile get_memorized_tile( const tripoint &p ) const;
        /** Memorizes a given tile in curses mode; finalize_terrain_memory_curses needs to be called after it */
        void memorize_symbol( const tripoint &pos, const long symbol );
        /** Returns last stored map tile in given location in curses mode */
        long get_memorized_symbol( const tripoint &p ) const;
        /** Returns the amount of tiles survivor can remember. */
        size_t max_memorized_tiles() const;
        void clear_memorized_tile( const tripoint &pos );

    private:
        map_memory player_map_memory;
        bool show_map_memory;
        /** Used in max_memorized_tiles to cache memory capacity. **/
        mutable time_point current_map_memory_turn = calendar::before_time_starts;
        mutable size_t current_map_memory_capacity = 0;
};

#endif
