#ifndef NAME_H
#define NAME_H

#include <string>
#include <vector>
#include <map>
#include <stdint.h>

typedef enum {
    nameIsMaleName = 1,
    nameIsFemaleName = 2,
    nameIsUnisexName = 3,
    nameIsGivenName = 4,
    nameIsFamilyName = 8,
    nameIsTownName = 16,
    nameIsFullName = 32,
    nameIsWorldName = 64
} nameFlags;

class NameGenerator;
class JsonObject;
class JsonIn;

class Name
{
    public:
        Name();
        Name( std::string name, uint32_t type );

        static NameGenerator &generator();
        static std::string generate( bool male );

        static std::string get( uint32_t searchFlags );

        std::string value() const {
            return _value;
        }
        uint32_t type() const {
            return _type;
        }

    private:
        std::string _value;
        uint32_t _type;
};

class NameGenerator
{
    public:
        static NameGenerator &generator() {
            static NameGenerator generator;

            return generator;
        }

        void load_name( JsonObject &jo );
        void load( JsonIn &jsin );

        std::string generateName( bool male );

        std::string getName( uint32_t searchFlags );
        void clear_names();
    private:
        NameGenerator();

        NameGenerator( NameGenerator const & );
        void operator=( NameGenerator const & );
        std::vector<uint32_t> uint32_tsFromFlags( uint32_t searchFlags ) const;

        std::map< uint32_t, std::vector<Name> > names;
};

void load_names_from_file( const std::string &filename );

#endif
