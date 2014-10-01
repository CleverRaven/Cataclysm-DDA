#ifndef NAME_H
#define NAME_H

#include <string>
#include <vector>
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

typedef uint32_t nameType;

class NameGenerator;

class Name
{
    public:
        Name();
        Name(std::string name, nameType type);

        static NameGenerator &generator();
        static std::string generate(bool male);

        static std::string get(nameType searchFlags);

        std::string value() const
        {
            return _value;
        }
        nameType type() const
        {
            return _type;
        }

    private:
        std::string _value;
        nameType _type;
};

class NameGenerator
{
    public:
        static NameGenerator &generator()
        {
            static NameGenerator generator;

            return generator;
        }

        void load_name(JsonObject &jo);

        std::string generateName(bool male);

        std::string getName(nameType searchFlags);
        void clear_names();
    private:
        NameGenerator();

        NameGenerator(NameGenerator const &);
        void operator=(NameGenerator const &);
        std::vector<nameType> nameTypesFromFlags(nameType searchFlags) const;

        std::map< nameType, std::vector<Name> > names;
};

void load_names_from_file(const std::string &filename);

#endif
