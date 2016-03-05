#ifndef MORALE_H
#define MORALE_H

#include "json.h"
#include <string>
#include "calendar.h"
#include "effect.h"
#include "bodypart.h"
#include "morale_types.h"

#include <functional>

class item;
class player;

struct itype;
struct morale_mult;

class player_morale
{
    public:
        player_morale() :
            level_is_valid( false ),
            took_prozak( false ),
            stylish( false ),
            super_fancy_bonus( 0 ) {};
        player_morale( player_morale && ) = default;
        player_morale( const player_morale & ) = default;
        player_morale &operator =( player_morale && ) = default;
        player_morale &operator =( const player_morale & ) = default;

        /** Adds morale to existing or creates one */
        void add( morale_type type, int bonus, int max_bonus = 0, int duration = MINUTES( 6 ),
                  int decay_start = MINUTES( 3 ), bool capped = false, const itype *item_type = nullptr );
        /** Adds permanent morale to existing or creates one */
        void add_permanent( morale_type type, int bonus, int max_bonus = 0,
                            bool capped = false, const itype *item_type = nullptr );
        /** Returns bonus from specified morale */
        int has( morale_type type, const itype *item_type = nullptr ) const;
        /** Removes specified morale */
        void remove( morale_type type, const itype *item_type = nullptr );
        /** Clears up all morale points */
        void clear();
        /** Returns overall morale level */
        int get_level() const;
        /** Ticks down morale counters and removes them */
        void decay( int ticks = 1 );
        /** Displays morale screen */
        void display();

    protected:
        friend class player;

        void on_trait_gain( const std::string &trait );
        void on_trait_loss( const std::string &trait );
        void on_item_wear( const item &it );
        void on_item_takeoff( const item &it );
        void on_effect_change( const effect &e );

        void store( JsonOut &jsout ) const;
        void load( JsonObject &jsin );

    private:
        class morale_point : public JsonSerializer, public JsonDeserializer
        {
            public:
                morale_point(
                    morale_type type = MORALE_NULL,
                    const itype *item_type = nullptr,
                    int bonus = 0,
                    int duration = MINUTES( 6 ),
                    int decay_start = MINUTES( 3 ),
                    int age = 0 ):
                    type( type ),
                    item_type( item_type ),
                    bonus( bonus ),
                    duration( duration ),
                    decay_start( decay_start ),
                    age( age ) {};

                using JsonDeserializer::deserialize;
                void deserialize( JsonIn &jsin ) override;
                using JsonSerializer::serialize;
                void serialize( JsonOut &json ) const override;

                std::string get_name() const;
                int get_net_bonus() const;
                int get_net_bonus( const morale_mult &mult ) const;
                bool is_expired() const;
                bool matches( morale_type _type, const itype *_item_type = nullptr ) const;

                void add( int new_bonus, int new_max_bonus, int new_duration,
                          int new_decay_start, bool new_cap );
                void decay( int ticks = 1 );

            private:
                morale_type type;
                const itype *item_type;

                int bonus;
                int duration;   // Zero duration == infinity
                int decay_start;
                int age;

                /**
                * Returns either new_time or remaining time (which one is greater).
                * Only returns new time if same_sign is true
                */
                int pick_time( int cur_time, int new_time, bool same_sign ) const;
        };
    private:
        morale_mult get_temper_mult() const;

        void set_wear_state( const item &it, bool worn );
        void set_prozak( bool new_took_prozak );

        void remove_if( const std::function<bool( const morale_point & )> &func );
        void remove_expired();
        void invalidate();
        void update_stylish_bonus();

    private:
        std::vector<morale_point> points;
        std::array<int, num_bp> covered;

        // Mutability is required for lazy initialization
        mutable int level;
        mutable bool level_is_valid;

        bool took_prozak;
        bool stylish;
        int super_fancy_bonus;
};

#endif
