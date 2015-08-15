#include <map>
#include <fstream>
#include <sstream>

#include "json.h"
#include "name.h"
#include "output.h"
#include "translations.h"
#include "rng.h"

NameGenerator::NameGenerator()
{

}

void NameGenerator::clear_names()
{
    names.clear();
}

void NameGenerator::load_name(JsonObject &jo)
{
    std::string name = jo.get_string("name");
    std::string usage = jo.get_string("usage");
    uint32_t type = 0;

    if (usage == "given") {
        type |= nameIsGivenName;
        name = pgettext("Given Name", name.c_str());
    } else if (usage == "family") {
        type |= nameIsFamilyName;
        name = pgettext("Family Name", name.c_str());
    } else if (usage == "universal") {
        type |= nameIsGivenName | nameIsFamilyName;
        name = pgettext("Either Name", name.c_str());
    } else if (usage == "backer") {
        type |= nameIsFullName;
        name = pgettext("Full Name", name.c_str());
    } else if (usage == "city") {
        type |= nameIsTownName;
        name = pgettext("City Name", name.c_str());
    } else if (usage == "world") {
        type |= nameIsWorldName;
        name = pgettext("World Name", name.c_str());
    }

    // Gender is optional
    if(jo.has_member("gender")) {
        std::string gender = jo.get_string("gender");

        if (gender == "male") {
            type |= nameIsMaleName;
        } else if (gender == "female") {
            type |= nameIsFemaleName;
        } else if (gender == "unisex") {
            type |= nameIsUnisexName;
        }
    }

    Name aName(name, type);

    // Add the name to the appropriate bucket
    names[type].push_back(aName);
}

// Find all name types satisfying the search flags.
// There can be more than one, i.e. if searchFlags == nameIsUnisexName
// this function will return [ nameIsMaleName, nameIsFemaleName ]
std::vector<uint32_t> NameGenerator::uint32_tsFromFlags(uint32_t searchFlags) const
{
    std::vector<uint32_t> types;

    for( std::map< uint32_t, std::vector<Name> >::const_iterator it = names.begin();
         it != names.end(); ++it ) {
        const uint32_t type = it->first;
        if ((searchFlags & type) == searchFlags) {
            types.push_back(type);
        }
    }

    return types;
}

// Get a random name with the specified flag
std::string NameGenerator::getName(uint32_t searchFlags)
{
    const std::vector<uint32_t> types = uint32_tsFromFlags(searchFlags);
    int nTypes = types.size();

    if (nTypes == 0) {
        // BUG, no matching name type found.
        return std::string( _("Tom") );
    }

    // Get number of matching names
    int nNames = 0;
    for (uint32_t type : types) {
        nNames += names[type].size();
    }

    // Choose a random name
    int choice = rng(0, nNames-1);
    for (uint32_t type : types) {
        std::vector<Name> &theseNames = names[type];
        if (choice < int(theseNames.size())) {
            return theseNames[choice].value();
        }
        choice -= theseNames.size();
    }

    // BUG, no matching name found.
    return std::string( _("Tom") );
}

std::string NameGenerator::generateName(bool male)
{
    uint32_t baseSearchFlags = male ? nameIsMaleName : nameIsFemaleName;
    //One in four chance to pull from the backer list, otherwise generate a name from the parts list
    if (one_in(4)) {
        return getName(baseSearchFlags | nameIsFullName);
    } else {
        //~ used for constructing names. swapping these will put family name first.
        return string_format(pgettext("Full Name", "%1$s %2$s"),
                             getName(baseSearchFlags | nameIsGivenName).c_str(),
                             getName(baseSearchFlags | nameIsFamilyName).c_str()
                            );
    }
}

NameGenerator &Name::generator()
{
    return NameGenerator::generator();
}

std::string Name::generate(bool male)
{
    return NameGenerator::generator().generateName(male);
}

std::string Name::get(uint32_t searchFlags)
{
    return NameGenerator::generator().getName(searchFlags);
}

Name::Name()
{
    _value = _("Tom");
    _type = 15;
}

Name::Name(std::string name, uint32_t type)
{
    _value = name;
    _type = type;
}

void load_names_from_file(const std::string &filename)
{
    std::ifstream data_file;
    data_file.open(filename.c_str(), std::ifstream::in | std::ifstream::binary);
    if(!data_file.good()) {
        throw std::runtime_error( std::string( "Could not read " ) + filename );
    }

    NameGenerator &gen = NameGenerator::generator();

    std::istringstream iss(
        std::string(
            (std::istreambuf_iterator<char>(data_file)),
            std::istreambuf_iterator<char>()
        )
    );
    JsonIn jsin(iss);
    data_file.close();

    // load em all
    jsin.start_array();
    while (!jsin.end_array()) {
        JsonObject json_name = jsin.get_object();
        gen.load_name(json_name);
    }
}

