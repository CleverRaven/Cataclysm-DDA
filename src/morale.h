#pragma once
#ifndef MORALE_H
#define MORALE_H

#include "string_id.h"
#include "calendar.h"
#include "bodypart.h"
#include "morale_types.h"

#include <stdlib.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

class item;
class JsonIn;
class JsonOut;
class JsonObject;
struct itype;
struct morale_mult;
class effect_type;
using efftype_id = string_id<effect_type>;
struct mutation_branch;
using trait_id = string_id<mutation_branch>;

class player_morale
{
    public:
        player_morale();

        player_morale( player_morale && ) = default;
        player_morale( const player_morale & ) = default;
        player_morale &operator =( player_morale && ) = default;
        player_morale &operator =( const player_morale & ) = default;

        /** Adds morale to existing or creates one */
        void add( morale_type type, int bonus, int max_bonus = 0, time_duration duration = 6_minutes,
                  time_duration decay_start = 3_minutes, bool capped = false, const itype *item_type = nullptr );
        /** Sets the new level for the permanent morale, or creates one */
        void set_permanent( const morale_type &type, int bonus, const itype *item_type = nullptr );
        /** Returns bonus from specified morale */
        int has( const morale_type &type, const itype *item_type = nullptr ) const;
        /** Removes specified morale */
        void remove( const morale_type &type, const itype *item_type = nullptr );
        /** Clears up all morale points */
        void clear();
        /** Returns overall morale level */
        int get_level() const;
        /** Ticks down morale counters and removes them */
        void decay( time_duration ticks = 1_turns );
        /** Displays morale screen */
        void display( double focus_gain );
        /** Returns false whether morale is inconsistent with the argument.
         *  Only permanent morale is checked */
        bool consistent_with( const player_morale &morale ) const;

        void on_mutation_gain( const trait_id &mid );
        void on_mutation_loss( const trait_id &mid );
        void on_stat_change( const std::string &stat, int value );
        void on_item_wear( const item &it );
        void on_item_takeoff( const item &it );
        void on_effect_int_change( const efftype_id &eid, int intensity, body_part bp = num_bp );

        void store( JsonOut &jsout ) const;
        void load( JsonObject &jsin );

    private:
        class morale_point
        {
            public:
                morale_point(
                    morale_type type = MORALE_NULL,
                    const itype *item_type = nullptr,
                    int bonus = 0,
                    int max_bonus = 0,
                    time_duration duration = 6_minutes,
                    time_duration decay_start = 3_minutes,
                    bool capped = false ) :

                    type( type ),
                    item_type( item_type ),
                    bonus( normalize_bonus( bonus, max_bonus, capped ) ),
                    duration( std::max( duration, 0_turns ) ),
                    decay_start( std::max( decay_start, 0_turns ) ),
                    age( 0_turns ) {}

                void deserialize( JsonIn &jsin );
                void serialize( JsonOut &json ) const;

                std::string get_name() const;
                int get_net_bonus() const;
                int get_net_bonus( const morale_mult &mult ) const;
                bool is_expired() const;
                bool is_permanent() const;
                bool matches( const morale_type &_type, const itype *_item_type = nullptr ) const;
                bool matches( const morale_point &mp ) const;

                void add( int new_bonus, int new_max_bonus, time_duration new_duration,
                          time_duration new_decay_start, bool new_cap );
                void decay( time_duration ticks = 1_turns );

            private:
                morale_type type;
                const itype *item_type;

                int bonus;
                time_duration duration;   // Zero duration == infinity
                time_duration decay_start;
                time_duration age;

                /**
                 * Returns either new_time or remaining time (which one is greater).
                 * Only returns new time if same_sign is true
                 */
                time_duration pick_time( time_duration cur_time, time_duration new_time, bool same_sign ) const;
                /**
                 * Returns normalized bonus if either max_bonus != 0 or capped == true
                 */
                int normalize_bonus( int bonus, int max_bonus, bool capped ) const;
        };
    protected:
        morale_mult get_temper_mult() const;

        void set_prozac( bool new_took_prozac );
        void set_prozac_bad( bool new_took_prozac_bad );
        void set_stylish( bool new_stylish );
        void set_worn( const item &it, bool worn );
        void set_mutation( const trait_id &mid, bool active );
        bool has_mutation( const trait_id &mid );

        void remove_if( const std::function<bool( const morale_point & )> &func );
        void remove_expired();
        void invalidate();

        void update_stylish_bonus();
        void update_squeamish_penalty();
        void update_masochist_bonus();
        void update_bodytemp_penalty( time_duration ticks );
        void update_constrained_penalty();

    private:
        std::vector<morale_point> points;

        struct body_part_data {
            unsigned int covered;
            unsigned int fancy;
            unsigned int filthy;
            int hot;
            int cold;

            body_part_data() :
                covered( 0 ),
                fancy( 0 ),
                filthy( 0 ),
                hot( 0 ),
                cold( 0 ) {};
        };
        std::array<body_part_data, num_bp> body_parts;
        body_part_data no_body_part;

        typedef std::function<void( player_morale *morale )> mutation_handler;
        struct mutation_data {
            public:
                mutation_data() = default;
                mutation_data( mutation_handler on_gain_and_loss ) :
                    on_gain( on_gain_and_loss ),
                    on_loss( on_gain_and_loss ),
                    active( false ) {};
                mutation_data( mutation_handler on_gain, mutation_handler on_loss ) :
                    on_gain( on_gain ),
                    on_loss( on_loss ),
                    active( false ) {};
                void set_active( player_morale *sender, bool new_active );
                bool get_active() const;
                void clear();
            private:
                mutation_handler on_gain;
                mutation_handler on_loss;
                bool active;
        };
        std::map<trait_id, mutation_data> mutations;

        std::map<std::string, int> super_fancy_items;

        // Mutability is required for lazy initialization
        mutable int level;
        mutable bool level_is_valid;

        bool took_prozac;
        bool took_prozac_bad;
        bool stylish;
        int perceived_pain;
};

#endif
