#ifndef START_LOCATION_H
#define START_LOCATION_H

#include <string>
#include <map>
#include <set>

class overmap;
class tinymap;
class player;
class JsonObject;
struct tripoint;

typedef std::map<std::string, class start_location> location_map;

class start_location
{
    public:
        start_location();

        std::string ident() const;
        std::string name() const;
        std::string target() const;
        const std::set<std::string> &flags() const;

        static location_map::iterator begin();
        static location_map::iterator end();
        static const start_location *find( const std::string ident );
        static void load_location( JsonObject &jsonobj );
        static void reset();

        /**
         * Setup the player start location on the overmaps.
         * It also initializes the map at that points using @ref prepare_map.
         * @return The player start location in global, absolute overmap terrain coordinates.
         */
        tripoint setup() const;
        /**
         * Place the player somewher ein th reality bubble (g->m).
         */
        void place_player( player &u ) const;
        /**
         * Burn random terrain / furniture with FLAMMABLE or FLAMMABLE_ASH tag.
         * Doors and windows are excluded.
         * @param rad safe radius area to prevent player spawn next to burning wall.
         * @param count number of fire on the map.
         */
        void burn( const tripoint &omtstart,
                   const size_t count, const int rad ) const;
        /**
         * Adds a map special, see mapgen.h and mapgen.cpp. Look at the namespace MapExtras.
         */
        void add_map_special( const tripoint &omtstart, const std::string &map_special ) const;

        void handle_heli_crash( player &u ) const;
    private:
        std::string _ident;
        std::string _name;
        std::string _target;
        std::set<std::string> _flags;

        void prepare_map( tinymap &m ) const;
};

#endif
