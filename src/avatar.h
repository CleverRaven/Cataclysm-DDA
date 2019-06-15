#pragma once
#ifndef AVATAR_H
#define AVATAR_H

#include "enums.h"
#include "player.h"

class avatar : public player
{
    public:
        avatar();

        void store( JsonOut &json ) const;
        void load( JsonObject &data );
        void serialize( JsonOut &josn ) const override;
        void deserialize( JsonIn &json ) override;
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
        void memorize_symbol( const tripoint &pos, const int symbol );
        /** Returns last stored map tile in given location in curses mode */
        int get_memorized_symbol( const tripoint &p ) const;
        /** Returns the amount of tiles survivor can remember. */
        size_t max_memorized_tiles() const;
        void clear_memorized_tile( const tripoint &pos );

        std::vector<mission *> get_active_missions() const;
        std::vector<mission *> get_completed_missions() const;
        std::vector<mission *> get_failed_missions() const;
        /**
         * Returns the mission that is currently active. Returns null if mission is active.
         */
        mission *get_active_mission() const;
        /**
         * Returns the target of the active mission or @ref overmap::invalid_tripoint if there is
         * no active mission.
         */
        tripoint get_active_mission_target() const;
        /**
         * Set which mission is active. The mission must be listed in @ref active_missions.
         */
        void set_active_mission( mission &cur_mission );
        /**
         * Called when a mission has been assigned to the player.
         */
        void on_mission_assignment( mission &new_mission );
        /**
         * Called when a mission has been completed or failed. Either way it's finished.
         * Check @ref mission::has_failed to see which case it is.
         */
        void on_mission_finished( mission &cur_mission );

        /**
         * Helper function for player::read.
         *
         * @param book Book to read
         * @param reasons Starting with g->u, for each player/NPC who cannot read, a message will be pushed back here.
         * @returns nullptr, if neither the player nor his followers can read to the player, otherwise the player/NPC
         * who can read and can read the fastest
         */
        const player *get_book_reader( const item &book, std::vector<std::string> &reasons ) const;
        /**
         * Helper function for get_book_reader
         * @warning This function assumes that the everyone is able to read
         *
         * @param book The book being read
         * @param reader the player/NPC who's reading to the caller
         * @param learner if not nullptr, assume that the caller and reader read at a pace that isn't too fast for him
         */
        int time_to_read( const item &book, const player &reader, const player *learner = nullptr ) const;
        /** Handles reading effects and returns true if activity started */
        bool read( int inventory_position, const bool continuous = false );
        /** Completes book reading action. **/
        void do_read( item &book );
        /** Note that we've read a book at least once. **/
        bool has_identified( const std::string &item_id ) const override;

        hint_rating rate_action_read( const item &it ) const;

        void wake_up();
        // Grab furniture / vehicle
        void grab( object_type grab_type, const tripoint &grab_point = tripoint_zero );
        object_type get_grab_type() const;
        /** Handles player vomiting effects */
        void vomit();

        /**
         * Try to steal an item from the NPC's inventory. May result in fail attempt, when NPC not notices you,
         * notices your steal attempt and getting angry with you, and you successfully stealing the item.
         * @param target Target NPC to steal from
         */
        void steal( npc &target );
    private:
        map_memory player_map_memory;
        bool show_map_memory;
        /** Used in max_memorized_tiles to cache memory capacity. **/
        mutable time_point current_map_memory_turn = calendar::before_time_starts;
        mutable size_t current_map_memory_capacity = 0;

        friend class debug_menu::mission_debug;
        /**
         * Missions that the player has accepted and that are not finished (one
         * way or the other).
         */
        std::vector<mission *> active_missions;
        /**
         * Missions that the player has successfully completed.
         */
        std::vector<mission *> completed_missions;
        /**
         * Missions that have failed while being assigned to the player.
         */
        std::vector<mission *> failed_missions;
        /**
         * The currently active mission, or null if no mission is currently in progress.
         */
        mission *active_mission;

        // Items the player has identified.
        std::unordered_set<std::string> items_identified;

        object_type grab_type;
};

#endif
