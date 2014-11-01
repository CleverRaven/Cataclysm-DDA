#ifndef START_LOCATION_H
#define START_LOCATION_H

#include "json.h"

#include <string>
#include <map>
#include <set>

class tinymap;

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

        void prepare_map(tinymap &m) const;

    private:
        std::string _ident;
        std::string _name;
        std::string _target;
        std::set<std::string> _flags;
};

#endif
