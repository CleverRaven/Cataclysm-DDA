#ifndef START_LOCATION_H
#define START_LOCATION_H

#include "json.h"

#include <string>
#include <map>
#include <set>

class overmap;
class tinymap;
class player;

typedef std::map<std::string, class start_location> location_map;

class start_location
{
    public:
        start_location();
        start_location( std::string ident, std::string name, std::string target );

        std::string ident() const;
        std::string name() const;
        std::string target() const;
        const std::set<std::string> &flags() const;

        static location_map::iterator begin();
        static location_map::iterator end();
        static start_location *find( const std::string ident );
        static void load_location( JsonObject &jsonobj );

        /**
         * Setup the player start location on the overmaps.
         * This sets cur_om, levc, levy, levz (members of the game class, see there).
         * It also initializes the map at that points using @ref prepare_map.
         */
        void setup( overmap *&cur_om, int &levx, int &levy, int &levz) const;
        /**
         * Place the player somewher ein th reality bubble (g->m).
         */
        void place_player( player &u ) const;

    private:
        std::string _ident;
        std::string _name;
        std::string _target;
        std::set<std::string> _flags;

        void prepare_map(tinymap &m) const;
};

#endif
